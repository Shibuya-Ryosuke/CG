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

		struct WindowData;

		// --- ImSequencer と ImCurveEdit を統合したデリゲートクラス ---
		struct WindowDelegate : public ImSequencer::SequenceInterface, public ImCurveEdit::Delegate {
			Impl* m_pImpl = nullptr;
			WindowData* m_pOwnerWindow = nullptr;

			// ImSequencer用のダミー/キャッシュバッファ
			// ImSequencerはアイテムごとの「開始/終了フレーム」を int* のポインタとして要求するため保持します
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

			// お使いのヘッダファイル (ImSequencer.h) のシグネチャに完全準拠
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

			// シーケンサー左側のラベル表示用
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

			// カーブエディタ用の頂点（X=フレーム, Y=値）に変換して一時キャッシュバッファを返す
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

			// グラフ上の点がドラッグされたとき
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override {
				if (!m_pImpl || m_pImpl->m_SelectedWindowIdx == -1) return pointIndex;
				auto& window = m_pImpl->m_SubWindows[m_pImpl->m_SelectedWindowIdx];
				auto& keys = window.keyFrames;
				if (pointIndex >= (int)keys.size()) return pointIndex;

				// Y座標(値)を更新
				if (curveIndex == 0) keys[pointIndex].value.x = value.y;
				else if (curveIndex == 1) keys[pointIndex].value.y = value.y;
				else keys[pointIndex].value.z = value.y;

				// ★重要：ドラッグ中の点のフレーム(X座標)に、ウィンドウの現在のフレームを追従させる
				keys[pointIndex].frame = static_cast<int32_t>(value.x);
				window.currentFrame = keys[pointIndex].frame;

				// フレームのドラッグ移動に合わせて自動ソート
				std::sort(keys.begin(), keys.end(), [](const KeyFrame& a, const KeyFrame& b) { return a.frame < b.frame; });

				return pointIndex;
			}

			// グラフ上を Ctrl + 左クリック等したときにキーフレームを追加する
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

			// アニメーション管理
			int32_t maxFrame = 60;        // 全体のフレーム数
			int32_t currentFrame = 0;    // 現在の再生・編集位置

			// このウィンドウが持つキーフレームたちの可変長配列
			std::vector<KeyFrame> keyFrames;
			// ウィンドウごとに独立したデリゲートを持たせる
			WindowDelegate delegate;
		};

		// ウィンドウ
		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;

		// モデルたち
		std::vector<std::pair<std::string, Model*>> m_pTargetModels;
	};

	// コンストラクタで生成
	AnimEdit::AnimEdit() {
		m_pImpl = new Impl();
	}
	// デストラクタで破棄
	AnimEdit::~AnimEdit() {
		delete m_pImpl;
	}

	void AnimEdit::Initialize(){}

	void AnimEdit::Update() {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// アニメーションエディタのメインウィンドウ
		ImGui::Begin("Animation Editor");
		{
			WindowManager();
			ModelOperate();

			// ★メインウィンドウ内のシーケンサーとグラフの描画コードは丸ごと削除しました
		}
		ImGui::End();

		// 量産された各サブウィンドウ（インスペクタとして動作）
		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {
			if (!impl->m_SubWindows[i].is_open) continue;

			auto& window = impl->m_SubWindows[i];
			std::string window_title = "インスペクタ: " + window.name + "##" + std::to_string(window.id);

			if (window.request_focus) {
				ImGui::SetNextWindowFocus();
				window.request_focus = false;
			}

			// ★グラフが入るため初期サイズを少し大きめに変更
			ImGui::SetNextWindowPos(ImVec2(100.0f, 600.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(550.0f, 500.0f), ImGuiCond_FirstUseEver);

			ImGui::Begin(window_title.c_str(), &window.is_open);

			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) ||
				(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseClicked(0))) {
				impl->m_SelectedWindowIdx = static_cast<int32_t>(i);
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
			ImGui::Separator();

			// --- ★ 各ウィンドウ独自のタイムライン（シーケンサー）の描画 ---
			int currentFrameItem = window.currentFrame;
			int selectedItem = -1;
			int firstFrame = 0;

			if (ImGui::TreeNode("translate (タイムライン)"))
			{
				if (ImGui::BeginChild("SequencerArea", ImVec2(0, 130), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
				{
					ImSequencer::Sequencer(&window.delegate, &currentFrameItem, nullptr, &selectedItem, &firstFrame,
						ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
				}
				ImGui::EndChild();

				// 開いている時だけ TreePop を呼ぶ
				ImGui::TreePop();
			}

			window.currentFrame = currentFrameItem;

			// --- ★ キーフレーム操作用補助ボタン ---
			if (ImGui::Button("現在のフレームにキーを挿入")) {
				auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const Impl::KeyFrame& k) { return k.frame == window.currentFrame; });
				if (it == window.keyFrames.end()) {
					Impl::KeyFrame newKey;
					newKey.frame = window.currentFrame;
					if (!impl->m_pTargetModels.empty() && impl->m_pTargetModels[0].second) {
						newKey.value = impl->m_pTargetModels[0].second->GetTranslate();
					}
					window.keyFrames.push_back(newKey);
					std::sort(window.keyFrames.begin(), window.keyFrames.end(), [](const Impl::KeyFrame& a, const Impl::KeyFrame& b) { return a.frame < b.frame; });
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("選択中のフレームのキーを削除")) {
				window.keyFrames.erase(std::remove_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const Impl::KeyFrame& k) {
					return k.frame == window.currentFrame;
					}), window.keyFrames.end());
			}

			// --- ★ 各ウィンドウ独自のカーブエディタ（グラフ）の描画 ---
			ImGui::Spacing();
			ImGui::Text("イージングカーブエディタ (赤:X, 緑:Y, 青:Z)  Ctrl+左クリックで点追加");
			if (ImGui::BeginChild("CurveEditorArea", ImVec2(0, 180), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar))
			{
				// Childウィンドウ内のサイズいっぱいにグラフを描画させる
				ImVec2 curveSize = ImGui::GetContentRegionAvail();
				ImCurveEdit::Edit(window.delegate, curveSize, window.id);
			}
			ImGui::EndChild();

			// --- 値のインスペクタ表示とモデルへのリアルタイム反映 ---
			auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(), [&](const Impl::KeyFrame& k) {
				return k.frame == window.currentFrame;
				});

			if (it != window.keyFrames.end()) {
				ImGui::TextColored(ImVec4(1, 1, 0, 1), "キーフレーム位置");
				float val[3] = { it->value.x, it->value.y, it->value.z };
				if (ImGui::DragFloat3("Value", val, 0.1f)) {
					it->value = Vector3{ val[0], val[1], val[2] };
				}
			} else {
				ImGui::Text("（キーフレームなし）");
			}

			if (!window.keyFrames.empty() && !impl->m_pTargetModels.empty() && impl->m_pTargetModels[0].second) {
				Model* targetModel = impl->m_pTargetModels[0].second;
				if (it != window.keyFrames.end()) {
					targetModel->SetTranslate(it->value);
				}
			}

			ImGui::End();
		}
	}

	void AnimEdit::WindowManager() {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// 新規ウィンドウの作成ボタン
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

			// ★ デリゲートポインタの初期設定をしてから追加
			Impl::WindowData newWindow;
			newWindow.id = allocated_id;
			newWindow.name = name;
			newWindow.is_open = true;

			impl->m_SubWindows.push_back(newWindow);

			// push_back完了後、確定したメモリ番地をデリゲートにバインドする
			auto& addedWindow = impl->m_SubWindows.back();
			addedWindow.delegate.m_pOwnerWindow = &addedWindow;
			addedWindow.delegate.m_pImpl = impl;
		}

		// セーブ
		ImGui::SameLine();
		if (ImGui::Button("設定を保存")) {
			SaveSettings();
		}
		// ロード
		ImGui::SameLine();
		if (ImGui::Button("設定を読み込み")) {
			LoadSettings();
		}

		ImGui::Separator();

		// 作成したウィンドウリストの一覧
		if (ImGui::TreeNode("ウィンドウリスト")) {
			// ドロップダウンのプレビュー文字（何も選択していないときは「選択してください」）
			std::string preview_text = "ウィンドウを選択";
			if ((impl->m_SelectedWindowIdx >= 0 && (impl->m_SelectedWindowIdx < (int)impl->m_SubWindows.size()))) {
				preview_text = impl->m_SubWindows[impl->m_SelectedWindowIdx].name;

				if (impl->m_SubWindows[impl->m_SelectedWindowIdx].is_open) {
					preview_text += " (Opened)";
				}
			}

			// ウィンドウ選択のタブ
			if (ImGui::BeginCombo("List", preview_text.c_str()))
			{
				for (size_t i = 0; i < impl->m_SubWindows.size(); i++)
				{
					bool is_selected = (impl->m_SelectedWindowIdx == (int)i);
					// すでに開いているものは名前の後ろに「(Opened)」と付けて分かりやすく
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

			// 再表示 (表示済みなら選択させる)
			ImGui::SameLine();
			if (ImGui::Button("開く"))
			{
				if (impl->m_SelectedWindowIdx != -1)
				{
					// 閉じている場合はフラグを true にして表示させる
					impl->m_SubWindows[impl->m_SelectedWindowIdx].is_open = true;
					// フォーカスを当てるフラグを立てる
					impl->m_SubWindows[impl->m_SelectedWindowIdx].request_focus = true;

					impl->m_SelectedWindowIdx = -1;
				}
			}

			// 選択したウィンドウを削除 (ポップアップ表示)
			if (ImGui::Button("削除"))
			{
				if (impl->m_SelectedWindowIdx != -1)
				{
					// 削除確認ポップアップを開く
					ImGui::OpenPopup("Delete Confirmation");
				}
			}
			// 削除確認ポップアップの文字やボタンの定義
			if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				// 選択されているウィンドウ名を取得
				std::string target_name = impl->m_SubWindows[impl->m_SelectedWindowIdx].name;

				// メッセージの表示
				ImGui::Text("%s を削除してよろしいですか？", target_name.c_str());
				ImGui::Separator();

				// 「実行」ボタン
				if (ImGui::Button("実行", ImVec2(120, 0))) {
					// 配列から完全消去（ここで実際に消す）
					impl->m_SubWindows.erase(impl->m_SubWindows.begin() + impl->m_SelectedWindowIdx);
					impl->m_SelectedWindowIdx = -1; // 選択をクリア

					ImGui::CloseCurrentPopup(); // ポップアップを閉じる
				}

				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();

				// 「キャンセル」ボタン
				if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup(); // 何もしきれずに閉じる
				}

				ImGui::EndPopup();
			}

			ImGui::TreePop();
		}
	}

	void AnimEdit::DrawUI() {
		//AnimEdit& instance = GetInstance();
		//Impl* impl = instance.m_pImpl;

		//// Animation Editor ウィンドウ生成
		//ImGui::Begin("Edit");
		//{
		//	// ウィンドウいっぱいにグラフを表示させるサイズ指定
		//	ImVec2 size = ImGui::GetContentRegionAvail();

  //          if (size.x < 100.0f) size.x = 100.0f;
  //          if (size.y < 200.0f) size.y = 200.0f;

  //          ImCurveEdit::Edit(impl->m_CombinedDelegate, size, 1);
		//}
		//ImGui::End();
	}

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
		// ★ 1. 「登録済みオブジェクト一覧」を大元のTreeNodeExにする（デフォルトで開く設定）
		if (ImGui::TreeNodeEx("登録済みオブジェクト", ImGuiTreeNodeFlags_DefaultOpen)) {

			if (impl->m_pTargetModels.empty()) {
				ImGui::Text("操作対象オブジェクト: なし");
			} else {
				bool isNoNameTreeOpen = false;
				bool hasCreatedNoNameTree = false;

				// 登録されているすべてのモデルをループで処理
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

			// ★ 大元の「登録済みオブジェクト一覧」のTreePop（if文の中身の一番最後）
			ImGui::TreePop();
		}
	}



	void AnimEdit::SetTargetModel(Model* model, const std::string& name) {
		if (model == nullptr) {
			Logger::LogWarning("[AnimEdit] (SetTargetModel)\nThe selected Model is nullptr.\n");
			return;
		}

		// 重複登録を防ぐチェック
		auto& models = GetInstance().m_pImpl->m_pTargetModels;
		// すでに同じポインタが登録されていないかチェック
		for (const auto& pair : models) {
			if (pair.second == model) return;
		}
		// 名前とポインタのペアを追加
		models.push_back(std::make_pair(name, model));
	}

	
}

#else
namespace RyoEngine {
	AnimEdit::AnimEdit() = default;
	AnimEdit::~AnimEdit() = default;
}
#endif
