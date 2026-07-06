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
		// キーフレームひとつあたりのデータ
		struct KeyFrame {
			int32_t frame = 0;                   // 何フレーム目か
			Vector3 value = { 0.0f,0.0f,0.0f };  // その時の座標(今はTranslate。いずれSRTに拡張)
			EasingType easing = EasingType::Lerp;
		};

		// --- ★ 追加：イージング適用関数 ---
		static float ApplyEasing(EasingType type, float t) {
			switch (type) {
			case EasingType::Lerp:          return t;
			case EasingType::EaseInQuad:    return t * t;
			case EasingType::EaseOutQuad:   return t * (2.0f - t);
			case EasingType::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
			case EasingType::EaseOutBounce:
				// 簡易的なバウンド数式（プロジェクト内のEasing::EaseOutBounce(t)等があればそちらに転送してもOKです）
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
				return (t >= 1.0f) ? 1.0f : 0.0f; // 1.0fに完全到達するまでは 0.0f（手前のキーフレーム位置）のまま
			}
		}

		// --- ★ 追加：Vector3の補間関数 ---
		static Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t) {
			return Vector3{
				start.x + (end.x - start.x) * t,
				start.y + (end.y - start.y) * t,
				start.z + (end.z - start.z) * t
			};
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

			// ImSequencer用のダミー/キャッシュバッファ
			int m_ItemStartFrame[3] = { 0, 0, 0 };
			int m_ItemEndFrame[3] = { 60, 60, 60 };
			int m_ItemType[3] = { 0, 0, 0 };

			// --- ImSequencer::SequenceInterface の実装 ---
			int GetFrameMin() const override { return 0; }
			int GetFrameMax() const override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return 60;
				return m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].maxFrame;
			}
			int GetItemCount() const override { return 3; } // Translate X, Y, Z の3つ

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
					uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 }; // R, G, B
					*color = colors[index];
				}
			}

			const char* GetItemLabel(int index) const override {
				static const char* labels[] = { "Translate.X", "Translate.Y", "Translate.Z" };
				return labels[index];
			}

			// --- ImCurveEdit::Delegate の実装 ---
			ImVec2& GetMin() override { static ImVec2 min(0.0f, -10.0f); return min; }
			ImVec2& GetMax() override {
				static ImVec2 max(60.0f, 100.0f);
				if (m_pImpl && m_pImpl->m_SelectedWindowIdx != -1) {
					max.x = static_cast<float>(m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].maxFrame);
				}
				return max;
			}

			size_t GetCurveCount() override { return 3; } // X, Y, Z の3つ
			bool IsVisible(size_t curveIndex) override { static_cast<void>(curveIndex); return true; }

			size_t GetPointCount(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return 0;
				return m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].keyFrames.size();
			}

			ImVec2* GetPoints(size_t curveIndex) override {
				static std::vector<ImVec2> pointsCache;
				pointsCache.clear();
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return nullptr;

				auto& keys = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx].keyFrames;
				for (const auto& k : keys) {
					float val = (curveIndex == 0) ? k.value.x : (curveIndex == 1) ? k.value.y : k.value.z;
					pointsCache.push_back(ImVec2(static_cast<float>(k.frame), val));
				}
				return pointsCache.data();
			}

			uint32_t GetCurveColor(size_t curveIndex) override {
				uint32_t colors[] = { 0xFF0000FF, 0xFF00FF00, 0xFFFF0000 }; // R, G, B
				return colors[curveIndex];
			}

			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return pointIndex;
				auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
				auto& keys = window.keyFrames;
				if (pointIndex >= (int)keys.size()) return pointIndex;

				if (curveIndex == 0) keys[pointIndex].value.x = value.y;
				else if (curveIndex == 1) keys[pointIndex].value.y = value.y;
				else keys[pointIndex].value.z = value.y;

				keys[pointIndex].frame = static_cast<int32_t>(value.x);
				window.currentFrame = keys[pointIndex].frame;

				std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

				return pointIndex;
			}

			void AddPoint(size_t curveIndex, ImVec2 value) override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return;
				auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];

				int32_t targetFrame = static_cast<int32_t>(value.x);
				auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const KeyFrame& k) { return k.frame == targetFrame; });

				if (it == window.keyFrames.end()) {
					KeyFrame newKey;
					newKey.frame = targetFrame;
					if (curveIndex == 0) newKey.value.x = value.y;
					else if (curveIndex == 1) newKey.value.y = value.y;
					else newKey.value.z = value.y;

					window.keyFrames.push_back(newKey);
					std::sort(window.keyFrames.begin(), window.keyFrames.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });
				}
			}
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

			std::vector<KeyFrame> keyFrames;
			WindowDelegate delegate;
		};

		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;

		std::vector<std::pair<std::string, Model*>> m_pTargetModels;

		// =========================================================================
		//  Implのインナースコープに関数を引っ越し（これでアクセス権問題を解消）
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
				ImGui::TreePop(); // TreePop は TreeNodeEx の場合も同様に必要です
			}
		}

		static void DrawKeyFrameButtons(WindowData& window, Impl* impl) {
			if (ImGui::Button("現在のフレームにキーを挿入")) {
				auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const KeyFrame& k) {
					return k.frame == window.currentFrame;
					});
				if (it == window.keyFrames.end()) {
					KeyFrame newKey;
					newKey.frame = window.currentFrame;
					if (!impl->m_pTargetModels.empty() && impl->m_pTargetModels[0].second) {
						newKey.value = impl->m_pTargetModels[0].second->GetTranslate();
					}
					window.keyFrames.push_back(newKey);
					std::sort(window.keyFrames.begin(), window.keyFrames.end(), [](const KeyFrame& a, const KeyFrame& b) {
						return a.frame < b.frame;
						});
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("選択中のフレームのキーを削除")) {
				window.keyFrames.erase(std::remove_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const KeyFrame& k) {
					return k.frame == window.currentFrame;
					}), window.keyFrames.end());
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
			auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const KeyFrame& k) {
				return k.frame == window.currentFrame;
				});

			if (it != window.keyFrames.end()) {
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "キーフレーム情報");
				float val[3] = { it->value.x, it->value.y, it->value.z };
				if (ImGui::DragFloat3("translate", val, 0.1f)) {
					it->value = Vector3{ val[0], val[1], val[2] };
				}

				// --- ★ 追加：イージングタイプ変更用のコンボボックス ---
				const char* easingNames[] = { "None", "Lerp", "EaseInQuad", "EaseOutQuad", "EaseInOutQuad", "EaseOutBounce" };
				int currentEasingIdx = static_cast<int>(it->easing);
				if (ImGui::Combo("Easing", &currentEasingIdx, easingNames, IM_ARRAYSIZE(easingNames))) {
					it->easing = static_cast<EasingType>(currentEasingIdx);
				}
			} else {
				ImGui::Text("（キーフレームなし）");
			}
		}

		static void DrawKeyFrameList(WindowData& window) {
			ImGui::Spacing();
			ImGui::Text("キーフレーム一覧 (クリックでジャンプ):");
			if (ImGui::BeginChild("KeyFrameListArea", ImVec2(0, 100), ImGuiChildFlags_Border)) {
				if (window.keyFrames.empty()) {
					ImGui::Text("（キーフレームが登録されていません）");
				} else {
					for (size_t k_idx = 0; k_idx < window.keyFrames.size(); ++k_idx) {
						auto& k = window.keyFrames[k_idx];
						bool is_active = (k.frame == window.currentFrame);

						std::string label = "キーフレーム " + std::to_string(k_idx + 1) + " (Frame: " + std::to_string(k.frame) + ")";
						if (is_active) label += " (active)";

						if (ImGui::Selectable(label.c_str(), is_active)) {
							window.currentFrame = k.frame;
						}

						if (is_active) {
							ImGui::SetScrollHereY();
						}
					}
				}
			}
			ImGui::EndChild();
		}

		static void UpdateAnimationAnimate(WindowData& window, Impl* impl) {
			if (window.keyFrames.empty() || impl->m_pTargetModels.empty() || !impl->m_pTargetModels[0].second) {
				return;
			}

			Model* targetModel = impl->m_pTargetModels[0].second;
			Vector3 finalTranslate = { 0.0f, 0.0f, 0.0f };

			if (window.currentFrame <= window.keyFrames.front().frame) {
				// 最初のキーフレーム以前なら、最初の値をそのまま適用
				finalTranslate = window.keyFrames.front().value;
			} else if (window.currentFrame >= window.keyFrames.back().frame) {
				// 最後のキーフレーム以降なら、最後の値をそのまま適用
				finalTranslate = window.keyFrames.back().value;
			} else {
				// 挟まれている2つのキーフレームを探索
				for (size_t k = 1; k < window.keyFrames.size(); ++k) {
					if (window.currentFrame <= window.keyFrames[k].frame) {
						const auto& prevKey = window.keyFrames[k - 1];
						const auto& nextKey = window.keyFrames[k];

						int32_t frameDiff = nextKey.frame - prevKey.frame;
						if (frameDiff > 0) {
							// 現在の進行度（0.0f ～ 1.0f）
							float t = static_cast<float>(window.currentFrame - prevKey.frame) / static_cast<float>(frameDiff);

							// 到達目標（現在向かっている側）のキーフレームが持っている easing を用いて補間割合を変換
							float easedT = ApplyEasing(nextKey.easing, t);

							// 計算された補間割合で線形補間
							finalTranslate = LerpVector3(prevKey.value, nextKey.value, easedT);
						} else {
							finalTranslate = nextKey.value;
						}
						break;
					}
				}
			}

			// 算出した座標をモデルにリアルタイム代入
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
				window.currentFrame = 0; // 最初に戻る
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

	// =========================================================================
	//  AnimEdit クラス実装
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

		// --- ★ 改善1: 1フレームの経過時間（DeltaTime）を安全に取得 ---
		float deltaTime = ImGui::GetIO().DeltaTime;

		// --- ★ 改善2: ウィンドウの状態に関わらず、すべてのアニメーションを常にバックグラウンドで更新 ---
		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			auto& window = impl->m_SubWindows[i];

			// 再生を停止し0フレーム目の位置へ
			if (!window.is_open) {
				if (window.isPlaying) {
					window.isPlaying = false;
					window.currentFrame = 0;
					Impl::UpdateAnimationAnimate(window, impl);
				}
				continue;
			}

			// AdvanceFrame に deltaTime を直接渡せるようにする（後述の修正）
			Impl::AdvanceFrame(window, deltaTime);
			Impl::UpdateAnimationAnimate(window, impl);
		}

		// メインのインスペクタウィンドウの描画
		ImGui::Begin("Animation Editor");
		{
			WindowManager();
			ModelOperate();
		}
		ImGui::End();

		// サブウィンドウの描画（ここでは描画の面倒だけを見る）
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

			json keys_arr = json::array();
			for (const auto& k : w.keyFrames) {
				json k_json;
				k_json["frame"] = k.frame;
				k_json["val_x"] = k.value.x;
				k_json["val_y"] = k.value.y;
				k_json["val_z"] = k.value.z;
				k_json["easing"] = static_cast<int32_t>(k.easing);
				keys_arr.push_back(k_json);
			}
			window_json["keyframes"] = keys_arr;
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

					if (item.contains("keyframes") && item["keyframes"].is_array()) {
						for (const auto& k_item : item["keyframes"]) {
							Impl::KeyFrame k;
							k.frame = k_item.value("frame", 0);
							k.value.x = k_item.value("val_x", 0.0f);
							k.value.y = k_item.value("val_y", 0.0f);
							k.value.z = k_item.value("val_z", 0.0f);
							k.easing = static_cast<EasingType>(k_item.value("easing", static_cast<int32_t>(EasingType::Lerp)));
							w.keyFrames.push_back(k);
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
		if (ImGui::TreeNode("登録済みオブジェクト")) {
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
							isNoNameTreeOpen = ImGui::TreeNode("Models");
							hasCreatedNoNameTree = true;
						}

						if (isNoNameTreeOpen) {
							if (ImGui::TreeNode((std::string("Model [") + std::to_string(i) + "]").c_str())) {
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

						if (ImGui::TreeNode(name.c_str())) {
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