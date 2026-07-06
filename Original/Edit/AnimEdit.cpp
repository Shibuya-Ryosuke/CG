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

		// キーフレームひとつあたりのデータ
		struct KeyFrame {
			int32_t frame = 0;                   // 何フレーム目か
			Vector3 value = { 0.0f,0.0f,0.0f };                 // ★ Vector3 から float に変更
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

		static void AdvanceFrame(WindowData& window, float deltaTime) {
			if (!window.isPlaying) return;

			window.frameTimer += deltaTime;

			// 1フレーム進むのに必要な時間
			float timePerFrame = 1.0f / window.fps;

			while (window.frameTimer >= timePerFrame) {
				window.frameTimer -= timePerFrame;
				window.currentFrame++;

				// ループ再生の処理
				if (window.currentFrame > window.maxFrame) {
					window.currentFrame = 0;
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

			const char* GetItemLabel(int index) const override {
				static const char* labels[] = { "Translate.X", "Translate.Y", "Translate.Z" };
				return labels[index];
			}

			ImVec2& GetMin() override { static ImVec2 min(0.0f, -10.0f); return min; }
			ImVec2& GetMax() override {
				static ImVec2 max(60.0f, 100.0f);
				if (m_pImpl && m_pImpl->m_SelectedWindowIdx != -1) {
					max.x = static_cast<float>(m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].maxFrame);
				}
				return max;
			}

			size_t GetCurveCount() override { return 3; }
			bool IsVisible(size_t curveIndex) override { static_cast<void>(curveIndex); return true; }

			uint32_t GetCurveColor(size_t curveIndex) override;

			// --- ★ 修正：軸ごとの個別の配列サイズを返す ---
			size_t GetPointCount(size_t curveIndex) override;

			// --- ★ 修正：軸ごとの個別キャッシュを生成 ---
			ImVec2* GetPoints(size_t curveIndex) override;

			// --- ★ 修正：該当する軸のキーフレームだけを編集 ---
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override;

			// --- ★ 修正：タイムラインからの直接追加も該当軸だけにする ---
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

			std::vector<KeyFrame> groupedKeyFrames[static_cast<size_t>(AxisGroup::MaxGroups)];

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

			WindowDelegate delegate;
		};

		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;

		std::vector<std::pair<std::string, Model*>> m_pTargetModels;

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

			if (ImGui::TreeNodeEx("translate (タイムライン)", ImGuiTreeNodeFlags_DefaultOpen)) {
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

				Vector3 modelPos = { 0.0f, 0.0f, 0.0f };
				if (!impl->m_pTargetModels.empty() && impl->m_pTargetModels[0].second) {
					modelPos = impl->m_pTargetModels[0].second->GetTranslate();
				}

				auto& keys = window.groupedKeyFrames[static_cast<size_t>(targetGroup)];

				// 同じフレームに既にキーがあれば上書き、なければ新規追加
				auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
					return k.frame == window.currentFrame;
					});

				if (it != keys.end()) {
					it->value = modelPos; // ★ Vector3を一気に上書き
				} else {
					KeyFrame newKey;
					newKey.frame = window.currentFrame;
					newKey.value = modelPos;
					newKey.group = targetGroup;
					keys.push_back(newKey);
					std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("選択中のフレームのキーを削除")) {
				AxisGroup targetGroup = window.GetCurrentTargetGroup();
				if (targetGroup != AxisGroup::None) {
					auto& keys = window.groupedKeyFrames[static_cast<size_t>(targetGroup)];
					keys.erase(std::remove_if(keys.begin(), keys.end(), [&](const KeyFrame& k) {
						return k.frame == window.currentFrame;
						}), keys.end());
				}
			}
		}

		static void DrawCurveEditor(WindowData& window) {
			ImGui::Spacing();
			ImGui::Text("イージングカーブエディタ (赤:X, 緑:Y, 青:Z)");
			if (ImGui::BeginChild("CurveEditorArea", ImVec2(0, 180), ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar)) {
				ImVec2 curveSize = ImGui::GetContentRegionAvail();
				ImCurveEdit::Edit(window.delegate, curveSize, window.id);
			}
			ImGui::EndChild();
		}

		static void DrawValueInspector(WindowData& window) {
			const char* easingNames[] = { "None", "Lerp", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutBounce" };
			bool hasKeyInCurrentFrame = false;

			// 全グループから現在のフレームにキーがあるか探す
			for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
				auto& keys = window.groupedKeyFrames[i];
				auto it = std::find_if(keys.begin(), keys.end(), [&](const KeyFrame& k) { return k.frame == window.currentFrame; });

				if (it != keys.end()) {
					if (!hasKeyInCurrentFrame) {
						ImGui::TextColored(ImVec4(1, 1, 0, 1), "キーフレーム情報 (現在のフレーム)");
						hasKeyInCurrentFrame = true;
					}

					AxisGroup g = static_cast<AxisGroup>(i);
					ImGui::Text("[%s グループ]", window.GetGroupName(g));

					// グループに応じて必要な軸のドラッグUIを出す
					if (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat("Translate.X", &it->value.x, 0.1f, -50.0f, 50.0f);
					}
					if (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat("Translate.Y", &it->value.y, 0.1f, -50.0f, 50.0f);
					}
					if (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ) {
						ImGui::DragFloat("Translate.Z", &it->value.z, 0.1f, -50.0f, 50.0f);
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

				// 全7グループを走査 (1:X ~ 7:XYZ)
				for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
					auto& keys = window.groupedKeyFrames[i];

					// ★ 空のグループは描画をスキップ！
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
					ImGui::Text("（キーフレームが登録されていません）");
				}
			}
			ImGui::EndChild();
		}

		// 特定の軸に関係するキーだけを一瞬集約して評価するヘルパー
		static float EvaluateAxisNew(int32_t currentFrame, const WindowData& window, int axisIndex, float defaultVal) {
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
					for (const auto& k : window.groupedKeyFrames[i]) {
						activeKeys.push_back(k);
					}
				}
			}

			if (activeKeys.empty()) return defaultVal;

			// フレーム順にソート
			std::sort(activeKeys.begin(), activeKeys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

			// --- あとは元の EvaluateAxis と同じ補間ロジック ---
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

		static void UpdateAnimationAnimate(WindowData& window, Impl* impl) {
			if (impl->m_pTargetModels.empty() || !impl->m_pTargetModels[0].second) return;
			Model* targetModel = impl->m_pTargetModels[0].second;
			Vector3 currentModelPos = targetModel->GetTranslate();

			Vector3 finalTranslate;
			finalTranslate.x = EvaluateAxisNew(window.currentFrame, window, 0, currentModelPos.x);
			finalTranslate.y = EvaluateAxisNew(window.currentFrame, window, 1, currentModelPos.y);
			finalTranslate.z = EvaluateAxisNew(window.currentFrame, window, 2, currentModelPos.z);
			targetModel->SetTranslate(finalTranslate);
		}

		static void DrawSubWindow(WindowData& window, size_t index, Impl* impl) {
			std::string window_title = "インスペクタ: " + window.name + "##" + std::to_string(window.id);

			if (window.request_focus) {
				ImGui::SetNextWindowFocus();
				window.request_focus = false;
			}

			ImGui::SetNextWindowPos(ImVec2(100.0f, 600.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(550.0f, 500.0f), ImGuiCond_FirstUseEver);

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
				}
			} else {
				if (ImGui::Button("> 再生")) {
					window.isPlaying = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("0フレームへ戻る")) {
				window.isPlaying = false;
				window.currentFrame = 0;
				window.frameTimer = 0.0f;
			}

			ImGui::Spacing();
			ImGui::Separator();

			DrawTimeline(window);
			DrawKeyFrameButtons(window, impl);
			DrawCurveEditor(window);
			DrawValueInspector(window);
			DrawKeyFrameList(window);

			ImGui::End();
		}
	};

	// --- 構造体の外側での WindowDelegate のメンバ関数定義 ---
	size_t AnimEdit::Impl::WindowDelegate::GetPointCount(size_t curveIndex) {
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return 0;
		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];

		size_t count = 0;
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) count += window.groupedKeyFrames[i].size();
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

		// 該当軸が含まれるグループのキーをキャッシュに集める
		for (size_t i = 1; i < static_cast<size_t>(AxisGroup::MaxGroups); ++i) {
			AxisGroup g = static_cast<AxisGroup>(i);
			bool include = false;
			if (curveIndex == 0) include = (g == AxisGroup::X || g == AxisGroup::XY || g == AxisGroup::XZ || g == AxisGroup::XYZ);
			if (curveIndex == 1) include = (g == AxisGroup::Y || g == AxisGroup::XY || g == AxisGroup::YZ || g == AxisGroup::XYZ);
			if (curveIndex == 2) include = (g == AxisGroup::Z || g == AxisGroup::XZ || g == AxisGroup::YZ || g == AxisGroup::XYZ);

			if (include) {
				for (const auto& k : window.groupedKeyFrames[i]) {
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
				for (size_t idx = 0; idx < window.groupedKeyFrames[i].size(); ++idx) {
					refs.push_back({ g, idx, window.groupedKeyFrames[i][idx].frame });
				}
			}
		}
		std::sort(refs.begin(), refs.end(), [&](const KeyRef& a, const KeyRef& b) {
			return window.groupedKeyFrames[static_cast<size_t>(a.group)][a.index].frame < window.groupedKeyFrames[static_cast<size_t>(b.group)][b.index].frame;
			});

		if (pointIndex >= (int)refs.size()) return pointIndex;

		auto& targetRef = refs[pointIndex];
		auto& key = window.groupedKeyFrames[static_cast<size_t>(targetRef.group)][targetRef.index];

		// 値の書き換え (Vector3の該当軸のみ書き換え)
		if (curveIndex == 0) key.value.x = value.y;
		if (curveIndex == 1) key.value.y = value.y;
		if (curveIndex == 2) key.value.z = value.y;

		// ★フレーム移動（ここを変更することで、グループ内の全軸が同期して一緒に動くようになります！）
		key.frame = static_cast<int32_t>(value.x);
		window.currentFrame = key.frame;

		// ソートし直す
		std::sort(window.groupedKeyFrames[static_cast<size_t>(targetRef.group)].begin(),
			window.groupedKeyFrames[static_cast<size_t>(targetRef.group)].end(),
			[](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

		return pointIndex;
	}

	void AnimEdit::Impl::WindowDelegate::AddPoint(size_t curveIndex, ImVec2 value) {
		if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return;
		auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];

		// タイムラインの空きをダブルクリックして追加した場合は、現在のチェックボックス選択に応じたグループに追加
		AxisGroup targetGroup = window.GetCurrentTargetGroup();
		if (targetGroup == AxisGroup::None) targetGroup = AxisGroup::XYZ; // デフォルト安全用

		int32_t targetFrame = static_cast<int32_t>(value.x);
		auto& keys = window.groupedKeyFrames[static_cast<size_t>(targetGroup)];
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

	void AnimEdit::Update() {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		float deltaTime = ImGui::GetIO().DeltaTime;

		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			auto& window = impl->m_SubWindows[i];

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

			json groups_arr = json::array();
			for (size_t i = 0; i < static_cast<size_t>(Impl::AxisGroup::MaxGroups); ++i) {
				json group_json = json::array();
				for (const auto& k : w.groupedKeyFrames[i]) {
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
			window_json["grouped_keyframes"] = groups_arr;
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

					if (item.contains("grouped_keyframes") && item["grouped_keyframes"].is_array()) {
						auto groups_arr = item["grouped_keyframes"];
						size_t max_g = std::min(groups_arr.size(), static_cast<size_t>(Impl::AxisGroup::MaxGroups));
						for (size_t i = 0; i < max_g; ++i) {
							w.groupedKeyFrames[i].clear();
							if (groups_arr[i].is_array()) {
								for (const auto& k_item : groups_arr[i]) {
									Impl::KeyFrame k;
									k.frame = k_item.value("frame", 0);
									k.value.x = k_item.value("val_x", 0.0f);
									k.value.y = k_item.value("val_y", 0.0f);
									k.value.z = k_item.value("val_z", 0.0f);
									k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
									k.group = static_cast<Impl::AxisGroup>(i);
									w.groupedKeyFrames[i].push_back(k);
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