#include "AnimEdit.h"
#include "../Base/Logger.h"
#include "../3D/Model.h"
#include "../Easing/Easing.h"
#include <vector>
#include <string>
#include <filesystem>
#include <fstream> 
#include <json.hpp>

using json = nlohmann::json;

#ifdef _DEBUG
#include "../ImGui/ImGuiAllInclude.h"

namespace RyoEngine {

	struct AnimEdit::Impl {
		enum class AxisGroup {
			None, // どこにも属さない（予備）
			X, Y, Z,
			XY, XZ, YZ,
			XYZ,
			MaxGroups
		};

		// ★ 追加：トランスフォームモード（SRT）の定義
		enum class TransformMode {
			Translate = 0,
			Rotate,
			Scale,
			MaxModes
		};

		// キーフレームひとつあたりのデータ
		struct KeyFrame {
			int32_t frame = 0;                   // 何フレーム目か
			Vector3 value = { 0.0f,0.0f,0.0f };
			EasingType easing = EasingType::Lerp;
			AxisGroup group = AxisGroup::XYZ;
		};

		// --- イージング適用関数 ---
		static float ApplyEasing(EasingType type, float t) {
			switch (type) {
			case EasingType::Lerp:          return t;
			case EasingType::EaseInQuad:    return t * t;
			case EasingType::EaseOutQuad:   return t * (2.0f - t);
			case EasingType::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
			case EasingType::EaseOutBounce:
				if (t < (1.0f / 2.75f)) {
					return 7.5625f * t * t;
				} else if (t < (2.0f / 2.75f)) {
					t -= (1.5f / 2.75f);
					return 7.5625f * t * t + 0.75f;
				} else if (t < (2.5f / 2.75f)) {
					t -= (2.25f / 2.75f);
					return 7.5625f * t * t + 0.9375f;
				} else {
					t -= (2.625f / 2.75f);
					return 7.5625f * t * t + 0.984375f;
				}
			case EasingType::None:
			default:
				return (t >= 1.0f) ? 1.0f : 0.0f;
			}
		}

		struct WindowData;

		static void AdvanceFrame(WindowData& window, float deltaTime) { // 引数の impl も不要になります
			if (!window.isPlaying) return;

			window.frameTimer += deltaTime;
			float timePerFrame = 1.0f / window.fps;
			while (window.frameTimer >= timePerFrame) {
				window.frameTimer -= timePerFrame;
				window.currentFrame++;

				if (window.currentFrame > window.maxFrame) {
					if (window.isLoop) {
						// 【無限ループ】
						window.currentFrame = 0;
						window.currentLoopCount++; // カウントだけ進めて0フレームからリピート
					} else {
						// 【回数指定再生】
						window.currentLoopCount++;
						if (window.currentLoopCount >= window.maxLoopCount) {
							// ★ すべての再生が終了した瞬間
							window.currentFrame = window.maxFrame;
							window.isPlaying = false;
							window.currentLoopCount = 0; // 0 に戻す

							// 自動再生が誤作動しないようにトリガー履歴をリセット
							window.lastTriggerState = false;
							break;
						} else {
							// 次の周回へ
							window.currentFrame = 0;
						}
					}
				}
			}
		}

		// --- ImSequencer と ImCurveEdit を統合したデリゲートクラス ---
		struct WindowDelegate : public ImSequencer::SequenceInterface, public ImCurveEdit::Delegate {
			Impl* m_pImpl = nullptr;
			WindowData* m_pOwnerWindow = nullptr;

			int m_ItemStartFrame[3] = { 0, 0, 0 };
			int m_ItemEndFrame[3] = { 60, 60, 60 };
			int m_ItemType[3] = { 0, 0, 0 };

			int GetFrameMin() const override { return 0; }
			int GetFrameMax() const override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return 60;
				return m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].maxFrame;
			}
			int GetItemCount() const override { return 3; }

			void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) {
					m_ItemStartFrame[index] = 0;
					m_ItemEndFrame[index] = 60;
				} else {
					auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
					m_ItemStartFrame[index] = 0;
					m_ItemEndFrame[index] = window.maxFrame;
				}

				if (start) *start = &m_ItemStartFrame[index];
				if (end) *end = &m_ItemEndFrame[index];
				if (type) *type = m_ItemType[index];
				if (color) {
					uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 };
					*color = colors[index];
				}
			}

			// ★ 修正：現在の編集モード（SRT）に応じてタイムラインのラベルを動的に切り替える
			const char* GetItemLabel(int index) const override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return "X";
				auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];

				if (window.currentTransformMode == static_cast<int>(TransformMode::Translate)) {
					static const char* labels[] = { "Translate.X", "Translate.Y", "Translate.Z" };
					return labels[index];
				} else if (window.currentTransformMode == static_cast<int>(TransformMode::Rotate)) {
					static const char* labels[] = { "Rotate.X", "Rotate.Y", "Rotate.Z" };
					return labels[index];
				} else {
					static const char* labels[] = { "Scale.X", "Scale.Y", "Scale.Z" };
					return labels[index];
				}
			}

			ImVec2& GetMin() override { static ImVec2 min(0.0f, -30.0f); return min; }
			ImVec2& GetMax() override {
				static ImVec2 max(60.0f, 50.0f);
				if (m_pImpl && m_pImpl->m_SelectedWindowIdx != -1) {
					max.x = static_cast<float>(m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].maxFrame);
				}
				return max;
			}

			size_t GetCurveCount() override { return 3; }
			bool IsVisible(size_t curveIndex) override { static_cast<void>(curveIndex); return true; }

			uint32_t GetCurveColor(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して配列サイズを返す ---
			size_t GetPointCount(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して個別キャッシュを生成 ---
			ImVec2* GetPoints(size_t curveIndex) override;

			// --- ★ 修正：モード(SRT)の次元を追加して該当する軸のキーフレームを編集 ---
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override;

			// --- ★ 修正：モード(SRT)の次元を追加して追加処理を行う ---
			void AddPoint(size_t curveIndex, ImVec2 value) override;
		};

		struct WindowData {
			int32_t id = 0;
			std::string name = "";
			bool is_open = false;
			bool request_focus = false;

			int32_t maxFrame = 60;
			int32_t currentFrame = 0;
			int32_t firstFrame = 0;

			bool isPlaying = false;
			float frameTimer = 0.0f;
			float fps = 60.0f;

			// チェックボックス用フラグ
			bool insertX = true;
			bool insertY = true;
			bool insertZ = true;

			// ★ 修正：[TransformMode(3)][AxisGroup(8)] の2次元配列に拡張
			std::vector<KeyFrame> groupedKeyFrames[static_cast<size_t>(TransformMode::MaxModes)][static_cast<size_t>(AxisGroup::MaxGroups)];

			// ★ 追加：現在の編集対象トランスフォームモード (0:Translate, 1:Rotate, 2:Scale)
			int currentTransformMode = 0;

			// チェックボックスの選択状態から AxisGroup を判定するヘルパー
			AxisGroup GetCurrentTargetGroup() const {
				if (insertX && insertY && insertZ) return AxisGroup::XYZ;
				if (insertX && insertY) return AxisGroup::XY;
				if (insertX && insertZ) return AxisGroup::XZ;
				if (insertY && insertZ) return AxisGroup::YZ;
				if (insertX) return AxisGroup::X;
				if (insertY) return AxisGroup::Y;
				if (insertZ) return AxisGroup::Z;
				return AxisGroup::None;
			}

			const char* GetGroupName(AxisGroup g) const {
				switch (g) {
				case AxisGroup::X:   return "X単体";
				case AxisGroup::Y:   return "Y単体";
				case AxisGroup::Z:   return "Z単体";
				case AxisGroup::XY:  return "XY同時";
				case AxisGroup::XZ:  return "XZ同時";
				case AxisGroup::YZ:  return "YZ同時";
				case AxisGroup::XYZ: return "XYZ同時";
				default:             return "None";
				}
			}

			bool useTrigger = false;               // トリガー機能を使うか
			std::string triggerFlagName = "None";  // 監視するフラグの名前
			bool triggerCondition = true;          // trueのとき開始するか、falseのときか
			bool lastTriggerState = false;         // 前フレームのフラグ状態

			// ★ 今回の変更：ループチェックと再生回数
			bool isLoop = true;                    // ループするかどうか
			int32_t maxLoopCount = 1;              // 指定された再生回数 (1以上で有効、0は無限ループなど)
			int32_t currentLoopCount = 0;          // 現在何回目の再生か

			// ★ 今回追加する「再生継続フラグ」用の設定データ
			bool useKeepRunning = false;           // 継続フラグによる再生維持を有効にするか
			std::string keepFlagName = "None";     // 監視する継続フラグの名前
			bool keepCondition = true;             // trueの間は再生するか、falseの間か
			
			// 途中で止められたとき0に戻すフラグ
			//bool returnToZeroOnStop = false;
			bool lastKeepState = false;

			WindowDelegate delegate;
		};

		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;

		std::vector<std::pair<std::string, Model*>> m_pTargetModels;

		std::vector<std::pair<std::string, bool*>> m_RegisteredFlags; // ★追加：登録フラグのリスト
		
		// ★追加：トリガー条件の監視ロジック関数
		static void CheckAnimationTrigger(WindowData& window, Impl* impl) {
			// ==========================================
	// ★ 毎フレーム最優先で走る「再生継続フラグ」のチェック
	// ==========================================
			if (window.useKeepRunning && window.keepFlagName != "None") {
				bool* pKeepFlag = nullptr;
				for (const auto& pair : impl->m_RegisteredFlags) {
					if (pair.first == window.keepFlagName) {
						pKeepFlag = pair.second;
						break;
					}
				}

				if (pKeepFlag) {
					bool currentKeepVal = *pKeepFlag;

					// --- 条件を満たしていない場合（強制停止） ---
					if (currentKeepVal != window.keepCondition) {
						if (window.isPlaying) {
							window.isPlaying = false;
							//if (window.returnToZeroOnStop) {
							//	window.frameTimer = 0.0f;
							//}
						}
						// 状態が変化したことを記録
						window.lastKeepState = false;
						window.lastTriggerState = false;
						return; // 開始トリガーの判定もさせない
					}

					// --- 条件を満たしている（安全な）場合 ---
					if (!window.useTrigger && !window.isPlaying) {
						// ★重要: 「前回のフレームでは条件を満たしていなかった(false)」かつ「今回は満たした(true)」
						// つまり、フラグが切り替わったまさにその瞬間だけ、1回だけ再生を開始する
						if (!window.lastKeepState) {
							//if (window.returnToZeroOnStop) {
							//	window.currentFrame = 0;
							//}
							window.isPlaying = true;
						}
					}

					// 条件を満たしている間は true を維持
					window.lastKeepState = true;
				}
			}

			// ==========================================
			// 既存の「開始トリガー」の監視ロジック（ここからは一切触らない）
			// ==========================================
			if (!window.useTrigger || window.triggerFlagName == "None") return;

			bool* pCurrentFlag = nullptr;
			for (const auto& pair : impl->m_RegisteredFlags) {
				if (pair.first == window.triggerFlagName) {
					pCurrentFlag = pair.second;
					break;
				}
			}
			if (!pCurrentFlag) return;

			bool currentVal = *pCurrentFlag;
			bool isTriggered = false;
			if (window.triggerCondition == true) {
				if (!window.lastTriggerState && currentVal) isTriggered = true; // false -> true
			} else {
				if (window.lastTriggerState && !currentVal) isTriggered = true; // true -> false
			}

			if (isTriggered) {
				// ★ 修正：「停止時に0に戻す」がOFF、かつ現在すでに途中まで進んでいるなら0に戻さない
				//if (window.returnToZeroOnStop || window.currentFrame >= window.maxFrame) {
				//	window.currentFrame = 0;
				//	window.frameTimer = 0.0f;
				//	window.currentLoopCount = 0;
				//}

				window.isPlaying = true; // 再生開始（または再開）！
			}
			window.lastTriggerState = currentVal;
		}
		// =========================================================================
		//  UI / アニメーション処理関数
		// =========================================================================
		static void DrawTimeline(WindowData& window) {
			ImGuiIO& io = ImGui::GetIO();
			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
					float mouseDeltaX = io.MouseDelta.x;
					if (mouseDeltaX != 0.0f) {
						window.firstFrame -= static_cast<int>(mouseDeltaX * 0.2f);
						if (window.firstFrame < 0) window.firstFrame = 0;
					}
				}
			}

			// ★ 修正：現在のモード名をヘッダーに表示
			const char* modeHeaderNames[] = { "タイムライン (translate)", "タイムライン (rotate)", "タイムライン (scale)" };
			if (ImGui::TreeNodeEx(modeHeaderNames[window.currentTransformMode], ImGuiTreeNodeFlags_DefaultOpen)) {
				if (ImGui::BeginChild("SequencerArea", ImVec2(0, 130), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar)) {
					int currentFrameItem = window.currentFrame;
					int selectedItem = -1;

					ImSequencer::Sequencer(&window.delegate, &currentFrameItem, nullptr, &selectedItem, &window.firstFrame,
						ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);

					window.currentFrame = currentFrameItem;
				}
				ImGui::EndChild();
				ImGui::TreePop();
			}
		}

		static void DrawKeyFrameButtons(WindowData& window, Impl* impl) {
			ImGui::Spacing();
			ImGui::Text("キー挿入対象（複数選択可）:");
			ImGui::Checkbox("X", &window.insertX); ImGui::SameLine();
			ImGui::Checkbox("Y", &window.insertY); ImGui::SameLine();
			ImGui::Checkbox("Z", &window.insertZ);

			if (ImGui::Button("現在のフレームにキーを挿入")) {
				AxisGroup targetGroup = window.GetCurrentTargetGroup();
				if (targetGroup == AxisGroup::None) return;

				// ★ 修正：現在のモードに応じた値をモデルから取得
				Vector3 modelVal = { 0.0f, 0.0f, 0.0f };
				if (!impl->m_pTargetModels.empty() && impl->m_pTargetModels[0].second) {
					Model* m = impl->m_pTargetModels[0].second;
					if (window.currentTransformMode == static_cast<int>(TransformMode::Translate)) modelVal = m->GetTranslate();
					else if (window.currentTransformMode == static_cast<int>(TransformMode::Rotate)) modelVal = m->GetRotate();
					else if (window.currentTransformMode == static_cast<int>(TransformMode::Scale)) modelVal = m->GetScale();
				}

				int mode = window.currentTransformMode;
				auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];

				// 同じフレームに既にキーがあれば上書き、なければ新規追加
				auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
					return k.frame == window.currentFrame;
					});

				if (it != keys.end()) {
					it->value = modelVal;
				} else {
					KeyFrame newKey;
					newKey.frame = window.currentFrame;
					newKey.value = modelVal;
					newKey.group = targetGroup;
					keys.push_back(newKey);
					std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("選択中のフレームのキーを削除")) {
				AxisGroup targetGroup = window.GetCurrentTargetGroup();
				if (targetGroup != AxisGroup::None) {
					int mode = window.currentTransformMode;
					auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];
					keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
						return k.frame == window.currentFrame;
						}), keys.end());
				}
			}
		}

		static void DrawCurveEditor(WindowData& window) {
			ImGui::Spacing();
			const char* curveEditorNames[] = {
				"グラフ (Translate) [赤:X, 緑:Y, 青:Z]",
				"グラフ (Rotate) [赤:X, 緑:Y, 青:Z]",
				"グラフ (Scale) [赤:X, 緑:Y, 青:Z]"
			};
			ImGui::Text("%s", curveEditorNames[window.currentTransformMode]);
			if (ImGui::BeginChild("CurveEditorArea", ImVec2(0, 180), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar)) {
				ImVec2 curveSize = ImGui::GetContentRegionAvail();

				// 子ウィンドウ自体の位置とサイズを取得
				ImVec2 windowPos = ImGui::GetWindowPos();
				ImVec2 windowSize = ImGui::GetWindowSize();

				// ★ 1. グラフの「暗い灰色のボックス」の正確な描画範囲（上下左右）を計算
				// ImCurveEditの内部の枠線（パディング）を考慮した矩形を作ります
				float paddingX = 8.0f;
				float paddingY = 4.0f; // ImCurveEdit の上下一枠分のデフォルト余白

				ImVec2 clipMin(windowPos.x + paddingX, windowPos.y + paddingY);
				ImVec2 clipMax(windowPos.x + windowSize.x - paddingX, windowPos.y + windowSize.y - paddingY);

				// グラフを描画
				ImCurveEdit::Edit(window.delegate, curveSize, window.id);

				// デリゲートからフレームの最小・最大値を取得
				float frameMin = static_cast<float>(window.delegate.GetFrameMin());
				float frameMax = static_cast<float>(window.delegate.GetFrameMax());

				if (frameMax > frameMin) {
					// ★ 動く量を調整する倍率（1動いたら0.9動かしたいなら 0.9f）
					float motionScale = 0.98825f;

					// 起点（例えば最小フレーム）からの「移動量」を計算し、そこに倍率をかける
					float baseFrame = frameMin; // どこを基準にしてズラすか（通常は最小フレーム）
					float movedFrame = (static_cast<float>(window.currentFrame) - baseFrame) * motionScale;

					// 倍率をかけた後の「現在の位置」を割り出す
					float adjustedFrame = baseFrame + movedFrame;

					// 補正したフレーム値を使って、グラフ全体の中の割合（0.0 〜 1.0）を計算
					float t = (adjustedFrame - frameMin) / (frameMax - frameMin);

					// 画面上のピクセル座標に変換
					float lineX = clipMin.x + (clipMax.x - clipMin.x) * t;
					if (lineX >= clipMin.x && lineX <= clipMax.x) {
						// 最前面の描画リストを取得
						ImDrawList* drawList = ImGui::GetForegroundDrawList();

						ImVec2 lineStart(lineX, clipMin.y);
						ImVec2 lineEnd(lineX, clipMax.y);

						// ★ 現在の子ウィンドウの DrawList から、親のスクロール等を考慮した「実際に画面に見えている表示領域」を取得
						ImDrawList* currentDrawList = ImGui::GetWindowDrawList();
						ImVec2 currentWindowClipMin = currentDrawList->GetClipRectMin();
						ImVec2 currentWindowClipMax = currentDrawList->GetClipRectMax();

						// グラフのボックス範囲（clipMin/Max）と、Windowの実際の表示領域の「重なり合う矩形」を計算する
						ImVec2 finalClipMin, finalClipMax;
						finalClipMin.x = (clipMin.x > currentWindowClipMin.x) ? clipMin.x : currentWindowClipMin.x;
						finalClipMin.y = (clipMin.y > currentWindowClipMin.y) ? clipMin.y : currentWindowClipMin.y;
						finalClipMax.x = (clipMax.x < currentWindowClipMax.x) ? clipMax.x : currentWindowClipMax.x;
						finalClipMax.y = (clipMax.y < currentWindowClipMax.y) ? clipMax.y : currentWindowClipMax.y;

						// クリップ領域が有効（潰れていない）場合のみ描画
						if (finalClipMin.x < finalClipMax.x && finalClipMin.y < finalClipMax.y) {
							// 計算した安全な範囲で最前面レイヤーにクリッピングをかける
							drawList->PushClipRect(finalClipMin, finalClipMax, false);

							// 白色（0xFFFFFFFF）で太さ 1.0f の細線を描画
							drawList->AddLine(lineStart, lineEnd, 0xFFFFFFFF, 1.0f);

							drawList->PopClipRect();
						}
					}
				}
			}
			ImGui::EndChild();
		}

		static void DrawValueInspector(WindowData& window) {
			const char* easingNames[] = { "None", "Lerp", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutBounce" };
			bool hasKeyInCurrentFrame = false;
			int mode = window.currentTransformMode;

			// 全グループから現在のフレームにキーがあるか探す
			for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
				auto& keys = window.groupedKeyFrames[mode][i];
				auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) { return k.frame == window.currentFrame; });

				if (it != keys.end()) {
					if (!hasKeyInCurrentFrame) {
						ImGui::Spacing();
						ImGui::TextColored(ImVec4(1, 1, 0, 1), "現在のキーフレーム情報");
						hasKeyInCurrentFrame = true;
					}

					AxisGroup g = static_cast<AxisGroup>(i);
					ImGui::Text("[ 所属グループ : %s ]", window.GetGroupName(g));

					// ★ 修正：現在のモードに応じてラベルを切り替え
					const char* labelX = (mode == 0) ? "Translate.X" : (mode == 1) ? "Rotate.X" : "Scale.X";
					const char* labelY = (mode == 0) ? "Translate.Y" : (mode == 1) ? "Rotate.Y" : "Scale.Y";
					const char* labelZ = (mode == 0) ? "Translate.Z" : (mode == 1) ? "Rotate.Z" : "Scale.Z";

					// グループに応じて必要な軸のドラッグUIを出す
					float dragSpeed = (mode == 0) ? 0.1f : 0.01f;

					if (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat(labelX, &it->value.x, dragSpeed, -360.0f, 360.0f);
					}
					if (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat(labelY, &it->value.y, dragSpeed, -360.0f, 360.0f);
					}
					if (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat(labelZ, &it->value.z, dragSpeed, -360.0f, 360.0f);
					}

					int easingIdx = static_cast<int>(it->easing);
					if (ImGui::Combo("Easing", &easingIdx, easingNames, IM_ARRAYSIZE(easingNames))) {
						it->easing = static_cast<EasingType>(easingIdx);
					}
					ImGui::Separator();
				}
			}

			if (!hasKeyInCurrentFrame) {
				ImGui::Text("（現在のフレームにキーフレームなし）");
			}
		}

		static void DrawKeyFrameList(WindowData& window) {
			ImGui::Spacing();
			ImGui::Text("グループ別キーフレーム一覧 (クリックでジャンプ):");
			if (ImGui::BeginChild("KeyFrameListArea", ImVec2(0, 120), ImGuiChildFlags_Border)) {

				bool hasAnyKey = false;
				int mode = window.currentTransformMode;

				// 全7グループを走査 (1:X ~ 7:XYZ)
				for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
					auto& keys = window.groupedKeyFrames[mode][i];

					// 空のグループは描画をスキップ
					if (keys.empty()) continue;

					hasAnyKey = true;
					AxisGroup g = static_cast<AxisGroup>(i);
					std::string groupLabel = std::string(window.GetGroupName(g)) + " (" + std::to_string(keys.size()) + ")";

					if (ImGui::TreeNode(groupLabel.c_str())) {
						for (const auto& key : keys) {
							bool is_active = (key.frame == window.currentFrame);
							std::string label = "フレーム: " + std::to_string(key.frame);
							if (is_active) label += " (active)";

							if (ImGui::Selectable(label.c_str(), is_active)) {
								window.currentFrame = key.frame;
							}
						}
						ImGui::TreePop();
					}
				}

				if (!hasAnyKey) {
					ImGui::Text("（選択中のモードにキーフレームが登録されていません）");
				}
			}
			ImGui::EndChild();
		}

		// ★ 修正：モード(SRT)指定を引数に含めて評価する
		static float EvaluateAxisNew(int32_t currentFrame, const WindowData& window, int mode, int axisIndex, float defaultVal) {
			// axisIndex -> 0:X, 1:Y, 2:Z

			// 今回の評価に関係するキーフレームを一時的に集める
			std::vector<KeyFrame> activeKeys;

			for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
				AxisGroup g = static_cast<AxisGroup>(i);

				// 軸Indexに応じて、その軸が含まれるグループのキーを拾う
				bool include = false;
				if (axisIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
				if (axisIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
				if (axisIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

				if (include) {
					for (const auto& k : window.groupedKeyFrames[mode][i]) {
						activeKeys.push_back(k);
					}
				}
			}

			if (activeKeys.empty()) return defaultVal;

			// フレーム順にソート
			std::sort(activeKeys.begin(), activeKeys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

			if (currentFrame <= activeKeys.front().frame) return (axisIndex == 0 ? activeKeys.front().value.x : (axisIndex == 1 ? activeKeys.front().value.y : activeKeys.front().value.z));
			if (currentFrame >= activeKeys.back().frame)  return (axisIndex == 0 ? activeKeys.back().value.x : (axisIndex == 1 ? activeKeys.back().value.y : activeKeys.back().value.z));

			for (size_t k = 1; k < activeKeys.size(); ++k) {
				if (currentFrame <= activeKeys[k].frame) {
					const auto& prevKey = activeKeys[k - 1];
					const auto& nextKey = activeKeys[k];

					int32_t frameDiff = nextKey.frame - prevKey.frame;
					if (frameDiff > 0) {
						float t = static_cast<float>(currentFrame - prevKey.frame) / static_cast<float>(frameDiff);
						float easedT = ApplyEasing(nextKey.easing, t);

						float pVal = (axisIndex == 0 ? prevKey.value.x : (axisIndex == 1 ? prevKey.value.y : prevKey.value.z));
						float nVal = (axisIndex == 0 ? nextKey.value.x : (axisIndex == 1 ? nextKey.value.y : nextKey.value.z));
						return pVal + (nVal - pVal) * easedT;
					}
					return (axisIndex == 0 ? nextKey.value.x : (axisIndex == 1 ? nextKey.value.y : nextKey.value.z));
				}
			}
			return defaultVal;
		}

		// ★ 修正：Translate, Rotate, Scale すべてを個別に評価して適用する
		static void UpdateAnimationAnimate(WindowData& window, Impl* impl) {
			if (impl->m_pTargetModels.empty() || !impl->m_pTargetModels[0].second) return;
			Model* targetModel = impl->m_pTargetModels[0].second;

			// 1. Translate 適用
			Vector3 currentModelPos = targetModel->GetTranslate();
			Vector3 finalTranslate;
			finalTranslate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 0, currentModelPos.x);
			finalTranslate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 1, currentModelPos.y);
			finalTranslate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Translate), 2, currentModelPos.z);
			targetModel->SetTranslate(finalTranslate);

			// 2. Rotate 適用
			Vector3 currentModelRot = targetModel->GetRotate();
			Vector3 finalRotate;
			finalRotate.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 0, currentModelRot.x);
			finalRotate.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 1, currentModelRot.y);
			finalRotate.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Rotate), 2, currentModelRot.z);
			targetModel->SetRotate(finalRotate);

			// 3. Scale 適用
			Vector3 currentModelScale = targetModel->GetScale();
			Vector3 finalScale;
			finalScale.x = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 0, currentModelScale.x);
			finalScale.y = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 1, currentModelScale.y);
			finalScale.z = EvaluateAxisNew(window.currentFrame, window, static_cast<int>(TransformMode::Scale), 2, currentModelScale.z);
			targetModel->SetScale(finalScale);
		}

		static void DrawSubWindow(WindowData& window, size_t index, Impl* impl) {
			std::string window_title = "インスペクタ: " + window.name + "##" + std::to_string(window.id);

			if (window.request_focus) {
				ImGui::SetNextWindowFocus();
				window.request_focus = false;
			}

			ImGui::SetNextWindowPos(ImVec2(100.0f, 600.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(550.0f, 600.0f), ImGuiCond_FirstUseEver);

			if (!ImGui::Begin(window_title.c_str(), &window.is_open)) {
				ImGui::End();
				return;
			}

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
				(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0))) {
				impl->m_SelectedWindowIdx = static_cast<int32_t>(index);
			}

			ImGui::Text("Window ID: %d", window.id);

			ImGui::PushItemWidth(150);
			ImGui::SliderInt("Frame", &window.currentFrame, 0, window.maxFrame);
			ImGui::SameLine();
			if (ImGui::InputInt("Max Frame", &window.maxFrame)) {
				if (window.maxFrame < 1) window.maxFrame = 1;
			}
			ImGui::PopItemWidth();

			ImGui::Spacing();
			if (window.isPlaying) {
				if (ImGui::Button("|| 一時停止")) {
					window.isPlaying = false;

					//// ★ 追加：手動停止時も設定に従う
					//if (window.returnToZeroOnStop) {
					//	window.currentFrame = 0;
					//	window.frameTimer = 0.0f;
					//}
				}
			} else {
				if (ImGui::Button("> 再生")) {
					// ★追加：もし指定回数をすべて再生し終えている状態なら、最初からリスタートする
					if (window.isLoop && window.maxLoopCount > 0 && window.currentLoopCount >= window.maxLoopCount) {
						window.currentFrame = 0;
						window.currentLoopCount = 0;
						window.frameTimer = 0.0f;
					}
					window.isPlaying = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("0フレームへ戻る")) {
				window.isPlaying = false;
				window.currentFrame = 0;
				window.frameTimer = 0.0f;
			}

			// ★追加：再生ボタンと編集モード（Combo）の間にトリガー設定UIを挟む
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ==========================================
			// 1. 【編集モード】（独立・常時表示）
			// ==========================================
			const char* transformModeNames[] = { "Translate (位置)", "Rotate (回転)", "Scale (拡縮)" };
			ImGui::PushItemWidth(200);
			ImGui::Combo("編集モード", &window.currentTransformMode, transformModeNames, IM_ARRAYSIZE(transformModeNames));
			ImGui::PopItemWidth();

			ImGui::Spacing();


			// ==========================================
			// 2. 【タイムライン】（独立・常時表示）
			// ==========================================
			DrawTimeline(window);

			ImGui::Spacing();


			// ==========================================
			// 3. 【キーフレーム】（新設した折りたたみヘッダー）
			// ==========================================
			if (ImGui::TreeNodeEx("キーフレーム", ImGuiTreeNodeFlags_DefaultOpen)) {
				DrawKeyFrameButtons(window, impl); // キー挿入対象などのボタン類
				DrawCurveEditor(window);           // カーブエディタ
				DrawValueInspector(window);        // インスペクタ
				DrawKeyFrameList(window);          // グループ別キーフレーム一覧
				ImGui::TreePop();
			} // ★ここで「キーフレーム」を閉じる


			// ==========================================
			// 4. 【アニメーション再生設定】（トリガーからループ設定まで）
			// ==========================================
			if (ImGui::CollapsingHeader("アニメーション再生設定")) {
				ImGui::Checkbox("トリガーによる開始を有効化", &window.useTrigger);
				if (window.useTrigger) {
					ImGui::Indent();
					std::string combo_preview = window.triggerFlagName;
					if (ImGui::BeginCombo("対象フラグ", combo_preview.c_str())) {
						if (ImGui::Selectable("None", window.triggerFlagName == "None")) { window.triggerFlagName = "None"; }
						for (const auto& pair : impl->m_RegisteredFlags) {
							if (ImGui::Selectable(pair.first.c_str(), window.triggerFlagName == pair.first)) {
								window.triggerFlagName = pair.first;
								window.lastTriggerState = *pair.second;
							}
						}
						ImGui::EndCombo();
					}
					ImGui::Text("開始条件:"); ImGui::SameLine();
					if (ImGui::RadioButton("True になったとき", window.triggerCondition == true)) { window.triggerCondition = true; }
					ImGui::SameLine();
					if (ImGui::RadioButton("False になったとき", window.triggerCondition == false)) { window.triggerCondition = false; }
					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();

				// --- 再生継続の設定 ---
				ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "[ 再生継続条件 ]");
				ImGui::Checkbox("フラグの状態による再生継続を有効化", &window.useKeepRunning);
				if (window.useKeepRunning) {
					ImGui::Indent();
					if (ImGui::BeginCombo("対象フラグ##Keep", window.keepFlagName.c_str())) {
						if (ImGui::Selectable("None", window.keepFlagName == "None")) { window.keepFlagName = "None"; }
						for (const auto& pair : impl->m_RegisteredFlags) {
							if (ImGui::Selectable(pair.first.c_str(), window.keepFlagName == pair.first)) {
								window.keepFlagName = pair.first;
							}
						}
						ImGui::EndCombo();
					}
					if (ImGui::RadioButton("True の間は再生##Keep", window.keepCondition == true)) { window.keepCondition = true; }
					ImGui::SameLine();
					if (ImGui::RadioButton("False の間は再生##Keep", window.keepCondition == false)) { window.keepCondition = false; }
					ImGui::Unindent();
				}

				ImGui::Spacing();
				ImGui::Separator();

				// --- ループ・終了の設定 ---
				ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "[ ループ・再生回数設定 ]");

				if (ImGui::Checkbox("ループする", &window.isLoop)) {
					if (window.isLoop) {
						window.maxLoopCount = 0;
					} else {
						window.maxLoopCount = 1;
					}
					window.currentFrame = 0;
					window.currentLoopCount = 0;
					window.frameTimer = 0.0f;
				}

				if (!window.isLoop) {
					ImGui::Indent();
					ImGui::PushItemWidth(100);
					if (ImGui::InputInt("再生回数", &window.maxLoopCount)) {
						if (window.maxLoopCount < 1) window.maxLoopCount = 1;
					}
					ImGui::PopItemWidth();

					int displayLoopCount = 0;
					if (window.isPlaying || (window.currentLoopCount > 0 && window.currentLoopCount < window.maxLoopCount)) {
						displayLoopCount = window.currentLoopCount + 1;
					}

					ImGui::Text("現在: %d / %d 回目", displayLoopCount, window.maxLoopCount);
					ImGui::Unindent();
				} else {
					ImGui::Indent();
					ImGui::Text("無限ループ中 (通算再生: %d 回)", window.currentLoopCount + 1);
					ImGui::Unindent();
				}
			} // ★ここで「アニメーション再生設定」を閉じる
			// ==========================================
			// 最後にサブウィンドウ自体を閉じる
			// ==========================================
			ImGui::NewLine();
			ImGui::End();
		}
	};

	// --- 構造体の外側での WindowDelegate のメンバ関数定義 ---
	size_t AnimEdit::Impl::WindowDelegate::GetPointCount(size_t curveIndex) {
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return 0;
		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
		int mode = window.currentTransformMode;

		size_t count = 0;
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) count += window.groupedKeyFrames[mode][i].size();
		}
		return count;
	}

	uint32_t AnimEdit::Impl::WindowDelegate::GetCurveColor(size_t curveIndex) {
		uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 }; // R, G, B
		return colors[curveIndex];
	}

	ImVec2* AnimEdit::Impl::WindowDelegate::GetPoints(size_t curveIndex) {
		static std::vector<ImVec2> pointsCache;
		pointsCache.clear();
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return nullptr;

		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
		int mode = window.currentTransformMode;

		// 該当軸が含まれるグループのキーをキャッシュに集める
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (const auto& k : window.groupedKeyFrames[mode][i]) {
					float val = (curveIndex == 0 ? k.value.x : (curveIndex == 1 ? k.value.y : k.value.z));
					pointsCache.push_back(ImVec2(static_cast<float>(k.frame), val));
				}
			}
		}
		// タイムライン描画のためにフレーム順でソート
		std::sort(pointsCache.begin(), pointsCache.end(), [](const ImVec2& a, const ImVec2& b) { return a.x < b.x; });
		return pointsCache.data();
	}

	int AnimEdit::Impl::WindowDelegate::EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) {
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return pointIndex;
		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
		int mode = window.currentTransformMode;

		// 現在表示されている点をフレーム順に追跡して、元のグループのデータを書き換える
		struct KeyRef { AxisGroup group; size_t index; int32_t originalFrame; };
		std::vector<KeyRef> refs;

		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (size_t idx = 0; idx < window.groupedKeyFrames[mode][i].size(); ++idx) {
					refs.push_back({ g, idx, window.groupedKeyFrames[mode][i][idx].frame });
				}
			}
		}
		std::sort(refs.begin(), refs.end(), [&](const KeyRef& a, const KeyRef& b) {
			return window.groupedKeyFrames[mode][static_cast<size_t>(a.group)][a.index].frame < window.groupedKeyFrames[mode][static_cast<size_t>(b.group)][b.index].frame;
			});

		if (pointIndex >= (int)refs.size()) return pointIndex;

		auto& targetRef = refs[pointIndex];
		auto& key = window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)][targetRef.index];

		// 値の書き換え (Vector3の該当軸のみ書き換え)
		if (curveIndex == 0) key.value.x = value.y;
		if (curveIndex == 1) key.value.y = value.y;
		if (curveIndex == 2) key.value.z = value.y;

		// フレーム移動
		key.frame = static_cast<int32_t>(value.x);
		window.currentFrame = key.frame;

		// ソートし直す
		std::sort(window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)].begin(),
			window.groupedKeyFrames[mode][static_cast<size_t>(targetRef.group)].end(),
			[](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

		return pointIndex;
	}

	void AnimEdit::Impl::WindowDelegate::AddPoint(size_t curveIndex, ImVec2 value) {
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return;
		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
		int mode = window.currentTransformMode;

		// タイムラインの空きをダブルクリックして追加した場合は、現在のチェックボックス選択に応じたグループに追加
		AxisGroup targetGroup = window.GetCurrentTargetGroup();
		if (targetGroup == AxisGroup::None) targetGroup = AxisGroup::XYZ; // デフォルト安全用

		int32_t targetFrame = static_cast<int32_t>(value.x);
		auto& keys = window.groupedKeyFrames[mode][static_cast<size_t>(targetGroup)];
		auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) { return k.frame == targetFrame; });

		if (it == keys.end()) {
			KeyFrame newKey;
			newKey.frame = targetFrame;
			if (curveIndex == 0) newKey.value.x = value.y;
			if (curveIndex == 1) newKey.value.y = value.y;
			if (curveIndex == 2) newKey.value.z = value.y;
			newKey.group = targetGroup;
			keys.push_back(newKey);
			std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
		}
	}


	// =========================================================================
	//  AnimEdit クラス本体の実装
	// =========================================================================
	AnimEdit::AnimEdit() {
		m_pImpl = new Impl();
	}

	AnimEdit::~AnimEdit() {
		delete m_pImpl;
	}

	void AnimEdit::Initialize() {}

	void AnimEdit::RegisterTriggerFlag(const std::string& name, bool* ptr) {
		if (!ptr) return;
		auto& flags = GetInstance().m_pImpl->m_RegisteredFlags;
		for (const auto& pair : flags) {
			if (pair.second == ptr) return; // 重複防止
		}
		flags.push_back(std::make_pair(name, ptr));
	}

	void AnimEdit::Update() {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		float deltaTime = ImGui::GetIO().DeltaTime;

		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			auto& window = impl->m_SubWindows[i];

			Impl::CheckAnimationTrigger(window, impl);

			if (!window.is_open) {
				if (window.isPlaying) {
					window.isPlaying = false;
					window.currentFrame = 0;
					Impl::UpdateAnimationAnimate(window, impl);
				}
				continue;
			}

			Impl::AdvanceFrame(window, deltaTime);
			Impl::UpdateAnimationAnimate(window, impl);
		}

		ImGui::Begin("Animation Editor");
		{
			WindowManager();

			ImGui::Spacing();

			ModelOperate();
		}
		ImGui::End();

		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			if (impl->m_SubWindows[i].is_open) {
				Impl::DrawSubWindow(impl->m_SubWindows[i], i, impl);
			}
		}
	}

	void AnimEdit::WindowManager() {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		if (ImGui::Button("新規作成")) {
			int32_t allocated_id = 1;
			while (true) {
				bool id_exists = false;
				for (const auto& w : impl->m_SubWindows) {
					if (w.id == allocated_id) {
						id_exists = true;
						break;
					}
				}
				if (!id_exists) break;
				allocated_id++;
			}

			std::string name = "新規ウィンドウ " + std::to_string(allocated_id);

			Impl::WindowData newWindow;
			newWindow.id = allocated_id;
			newWindow.name = name;
			newWindow.is_open = true;
			newWindow.currentTransformMode = 0; // デフォルトで Translate

			impl->m_SubWindows.push_back(newWindow);

			auto& addedWindow = impl->m_SubWindows.back();
			addedWindow.delegate.m_pOwnerWindow = &addedWindow;
			addedWindow.delegate.m_pImpl = impl;
		}

		ImGui::SameLine();
		if (ImGui::Button("設定を保存")) {
			SaveSettings();
		}
		ImGui::SameLine();
		if (ImGui::Button("設定を読み込み")) {
			LoadSettings();
		}

		ImGui::Separator();

		if (ImGui::TreeNodeEx("ウィンドウリスト", ImGuiTreeNodeFlags_DefaultOpen)) {
			std::string preview_text = "ウィンドウを選択";
			if ((impl->m_SelectedWindowIdx >= 0 && (impl->m_SelectedWindowIdx < (int)impl->m_SubWindows.size()))) {
				preview_text = impl->m_SubWindows[impl->m_SelectedWindowIdx].name;

				if (impl->m_SubWindows[impl->m_SelectedWindowIdx].is_open) {
					preview_text += " (Opened)";
				}
			}

			if (ImGui::BeginCombo("List", preview_text.c_str())) {
				for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
					bool is_selected = (impl->m_SelectedWindowIdx == (int)i);
					std::string item_name = impl->m_SubWindows[i].name;
					if (impl->m_SubWindows[i].is_open) {
						item_name += " (Opened)";
					}

					if (ImGui::Selectable(item_name.c_str(), is_selected)) {
						impl->m_SelectedWindowIdx = (int)i;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::SameLine();
			if (ImGui::Button("開く")) {
				if (impl->m_SelectedWindowIdx != -1) {
					impl->m_SubWindows[impl->m_SelectedWindowIdx].is_open = true;
					impl->m_SubWindows[impl->m_SelectedWindowIdx].request_focus = true;
					impl->m_SelectedWindowIdx = -1;
				}
			}

			if (ImGui::Button("削除")) {
				if (impl->m_SelectedWindowIdx != -1) {
					ImGui::OpenPopup("Delete Confirmation");
				}
			}

			if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				std::string target_name = impl->m_SubWindows[impl->m_SelectedWindowIdx].name;

				ImGui::Text("%s を削除してよろしいですか？", target_name.c_str());
				ImGui::Separator();

				if (ImGui::Button("実行", ImVec2(120, 0))) {
					impl->m_SubWindows.erase(impl->m_SubWindows.begin() + impl->m_SelectedWindowIdx);
					impl->m_SelectedWindowIdx = -1;
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();

				if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::TreePop();
		}
	}

	void AnimEdit::DrawUI() {}

	// ★ 修正：JSON形式を [TransformMode][AxisGroup] の入れ子構造に拡張して保存
	void AnimEdit::SaveSettings(const char* filePath) {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;
		std::filesystem::path p(filePath);
		if (p.has_parent_path()) { std::filesystem::create_directories(p.parent_path()); }

		json j = json::array();
		for (const auto& w : impl->m_SubWindows) {
			json window_json;
			window_json["id"] = w.id;
			window_json["name"] = w.name;
			window_json["is_open"] = false;
			window_json["max_frame"] = w.maxFrame;
			window_json["current_frame"] = w.currentFrame;
			window_json["insert_x"] = w.insertX;
			window_json["insert_y"] = w.insertY;
			window_json["insert_z"] = w.insertZ;
			window_json["current_transform_mode"] = w.currentTransformMode;
			
			window_json["use_trigger"] = w.useTrigger;                  // ★追加
			window_json["trigger_flag_name"] = w.triggerFlagName;        // ★追加
			window_json["trigger_condition"] = w.triggerCondition;
			// ★追加
			window_json["is_loop"] = w.isLoop;
			window_json["max_loop_count"] = w.maxLoopCount;

			window_json["use_keep_running"] = w.useKeepRunning;
			window_json["keep_flag_name"] = w.keepFlagName;
			window_json["keep_condition"] = w.keepCondition;
			//window_json["return_to_zero_on_stop"] = w.returnToZeroOnStop; // ★追加

			json modes_arr = json::array();
			for (size_t m = 0; m < static_cast<size_t>(Impl::TransformMode::MaxModes); ++m) {
				json groups_arr = json::array();
				for (size_t i = 0; i < static_cast<size_t>(Impl::AxisGroup::MaxGroups); ++i) {
					json group_json = json::array();
					for (const auto& k : w.groupedKeyFrames[m][i]) {
						json k_json;
						k_json["frame"] = k.frame;
						k_json["val_x"] = k.value.x;
						k_json["val_y"] = k.value.y;
						k_json["val_z"] = k.value.z;
						k_json["easing"] = static_cast<int32_t>(k.easing);
						group_json.push_back(k_json);
					}
					groups_arr.push_back(group_json);
				}
				modes_arr.push_back(groups_arr);
			}
			window_json["grouped_keyframes_srt"] = modes_arr; // 互換性のためにキー名を新調
			j.push_back(window_json);
		}

		std::ofstream file(filePath);
		if (file.is_open()) {
			file << j.dump(4);
			Logger::LogSuccess("[Animation Editor] Save Successed.");
		} else {
			Logger::LogWarning("[Animation Editor] Save failed.");
		}
	}

	// ★ 修正：拡張したJSON構造からデータを正しく展開・復元
	void AnimEdit::LoadSettings(const char* filePath) {
		std::ifstream file(filePath);
		if (!file.is_open()) return;
		json j;
		try { file >> j; }
		catch (...) { return; }

		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;
		impl->m_SubWindows.clear();
		impl->m_SelectedWindowIdx = -1;

		if (j.is_array()) {
			for (const auto& item : j) {
				if (item.contains("id") && item.contains("name")) {
					Impl::WindowData w;
					w.id = item["id"].get<int32_t>();
					w.name = item["name"].get<std::string>();
					w.is_open = false;
					w.maxFrame = item.value("max_frame", 60);
					w.currentFrame = item.value("current_frame", 0);
					w.insertX = item.value("insert_x", true);
					w.insertY = item.value("insert_y", true);
					w.insertZ = item.value("insert_z", true);
					w.currentTransformMode = item.value("current_transform_mode", 0);
					
					w.useTrigger = item.value("use_trigger", false);                     // ★追加
					w.triggerFlagName = item.value("trigger_flag_name", "None");         // ★追加
					w.triggerCondition = item.value("trigger_condition", true);          // ★追加
					w.lastTriggerState = false;
					// ★ 以下を追記
					w.isLoop = item.value("is_loop", true);
					w.maxLoopCount = item.value("max_loop_count", 1);
					w.currentLoopCount = 0;

					w.useKeepRunning = item.value("use_keep_running", false);
					w.keepFlagName = item.value("keep_flag_name", "None");
					w.keepCondition = item.value("keep_condition", true);
					//w.returnToZeroOnStop = item.value("return_to_zero_on_stop", false); // ★追加

					// データのクリア
					for (size_t m = 0; m < static_cast<size_t>(Impl::TransformMode::MaxModes); ++m) {
						for (size_t i = 0; i < static_cast<size_t>(Impl::AxisGroup::MaxGroups); ++i) {
							w.groupedKeyFrames[m][i].clear();
						}
					}

					if (item.contains("grouped_keyframes_srt") && item["grouped_keyframes_srt"].is_array()) {
						auto modes_arr = item["grouped_keyframes_srt"];
						size_t max_m = std::min(modes_arr.size(), static_cast<size_t>(Impl::TransformMode::MaxModes));

						for (size_t m = 0; m < max_m; ++m) {
							if (!modes_arr[m].is_array()) continue;
							auto groups_arr = modes_arr[m];
							size_t max_g = std::min(groups_arr.size(), static_cast<size_t>(Impl::AxisGroup::MaxGroups));

							for (size_t i = 0; i < max_g; ++i) {
								if (groups_arr[i].is_array()) {
									for (const auto& k_item : groups_arr[i]) {
										Impl::KeyFrame k;
										k.frame = k_item.value("frame", 0);
										k.value.x = k_item.value("val_x", 0.0f);
										k.value.y = k_item.value("val_y", 0.0f);
										k.value.z = k_item.value("val_z", 0.0f);
										k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
										k.group = static_cast<Impl::AxisGroup>(i);
										w.groupedKeyFrames[m][i].push_back(k);
									}
								}
							}
						}
					}
					// 互換性救済：もし古いバージョンのセーブデータがあった場合、Translateに展開
					else if (item.contains("grouped_keyframes") && item["grouped_keyframes"].is_array()) {
						auto groups_arr = item["grouped_keyframes"];
						size_t max_g = std::min(groups_arr.size(), static_cast<size_t>(Impl::AxisGroup::MaxGroups));
						for (size_t i = 0; i < max_g; ++i) {
							if (groups_arr[i].is_array()) {
								for (const auto& k_item : groups_arr[i]) {
									Impl::KeyFrame k;
									k.frame = k_item.value("frame", 0);
									k.value.x = k_item.value("val_x", 0.0f);
									k.value.y = k_item.value("val_y", 0.0f);
									k.value.z = k_item.value("val_z", 0.0f);
									k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
									k.group = static_cast<Impl::AxisGroup>(i);
									w.groupedKeyFrames[0][i].push_back(k); // 0 = Translate
								}
							}
						}
					}

					impl->m_SubWindows.push_back(w);
				}
			}

			for (auto& w : impl->m_SubWindows) {
				w.delegate.m_pOwnerWindow = &w;
				w.delegate.m_pImpl = impl;
			}
		}
		if (!impl->m_SubWindows.empty()) impl->m_SelectedWindowIdx = 0;
		Logger::LogSuccess("[Animation Editor] Load Successed.");
	}

	void AnimEdit::ModelOperate() {
		Impl* impl = GetInstance().m_pImpl;

		ImGui::Spacing();
		if (ImGui::TreeNodeEx("登録済みオブジェクト", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (impl->m_pTargetModels.empty()) {
				ImGui::Text("操作対象オブジェクト: なし");
			} else {
				bool isNoNameTreeOpen = false;
				bool hasCreatedNoNameTree = false;

				for (size_t i = 0; i < impl->m_pTargetModels.size(); ++i) {
					const std::string& name = impl->m_pTargetModels[i].first;
					Model* model = impl->m_pTargetModels[i].second;
					if (!model) continue;

					ImGui::PushID(static_cast<int>(i));

					if (name == "NoName") {
						if (!hasCreatedNoNameTree) {
							isNoNameTreeOpen = ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_DefaultOpen);
							hasCreatedNoNameTree = true;
						}

						if (isNoNameTreeOpen) {
							if (ImGui::TreeNodeEx((std::string("Model [") + std::to_string(i) + "]").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
								Vector3 translate = model->GetTranslate();
								float pos[3] = { translate.x, translate.y, translate.z };
								if (ImGui::DragFloat3("translate", pos, 0.1f)) {
									model->SetTranslate({ pos[0], pos[1], pos[2] });
								}

								Vector3 rotate = model->GetRotate();
								float rot[3] = { rotate.x, rotate.y, rotate.z };
								if (ImGui::DragFloat3("rotate", rot, 0.1f)) {
									model->SetRotate({ rot[0], rot[1], rot[2] });
								}

								Vector3 scale = model->GetScale();
								float scl[3] = { scale.x, scale.y, scale.z };
								if (ImGui::DragFloat3("scale", scl, 0.1f)) {
									model->SetScale({ scl[0], scl[1], scl[2] });
								}

								ImGui::TreePop();
							}
						}
					} else {
						if (isNoNameTreeOpen) {
							ImGui::TreePop();
							isNoNameTreeOpen = false;
							hasCreatedNoNameTree = false;
						}

						if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
							Vector3 translate = model->GetTranslate();
							float pos[3] = { translate.x, translate.y, translate.z };
							if (ImGui::DragFloat3("translate", pos, 0.1f)) {
								model->SetTranslate({ pos[0], pos[1], pos[2] });
							}

							Vector3 rotate = model->GetRotate();
							float rot[3] = { rotate.x, rotate.y, rotate.z };
							if (ImGui::DragFloat3("rotate", rot, 0.1f)) {
								model->SetRotate({ rot[0], rot[1], rot[2] });
							}

							Vector3 scale = model->GetScale();
							float scl[3] = { scale.x, scale.y, scale.z };
							if (ImGui::DragFloat3("scale", scl, 0.1f)) {
								model->SetScale({ scl[0], scl[1], scl[2] });
							}

							ImGui::TreePop();
						}
					}
					ImGui::PopID();
				}

				if (isNoNameTreeOpen) {
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}

	void AnimEdit::SetTargetModel(Model* model, const std::string& name) {
		if (model == nullptr) {
			Logger::LogWarning("[AnimEdit] (SetTargetModel)\nThe selected Model is nullptr.\n");
			return;
		}

		auto& models = GetInstance().m_pImpl->m_pTargetModels;
		for (const auto& pair : models) {
			if (pair.second == model) return;
		}
		models.push_back(std::make_pair(name, model));
	}
}

#else
namespace RyoEngine {
	AnimEdit::AnimEdit() = default;
	AnimEdit::~AnimEdit() = default;
}
#endif