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
		struct WindowData {
			int32_t id = 0;
			std::string name = "";
			bool is_open = false;
			bool request_focus = false;

			// アニメーション管理
			int32_t maxFrame = 60;        // 全体のフレーム数
			int32_t currentFrame = 0;    // 現在の再生・編集位置

			// キーフレームひとつあたりのデータ
			struct KeyFrame {
				int32_t frame = 0;                   // 何フレーム目か
				Vector3 value = { 0.0f,0.0f,0.0f };  // その時の座標(今はTranslate。いずれSRTに拡張)
				EasingType easing = EasingType::Lerp;
			};
			// このウィンドウが持つキーフレームたちの可変長配列
			std::vector<KeyFrame> keyFrames;
		};

		// ウィンドウ
		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;

		// モデルたち
		std::vector<std::pair<std::string, Model*>> m_pTargetModels;


		struct CurveDelegate : public ImCurveEdit::Delegate {
			// データを保持する配列（初期値）
			ImVec2 mPoints[3] = {
				ImVec2(0.0f, 0.0f),
				ImVec2(0.5f, 0.5f),
				ImVec2(1.0f, 1.0f)
			};


			ImVec2& GetMin() override { static ImVec2 min(0.0f, 0.0f); return min; }
			ImVec2& GetMax() override { static ImVec2 max(1.0f, 1.0f); return max; }
			size_t GetCurveCount() override { return 1; }
			bool IsVisible(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return true;
			}
			size_t GetPointCount(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 3;
			}
			ImVec2* GetPoints(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return mPoints;
			}
			uint32_t GetCurveColor(size_t curveIndex) override {
				static_cast<void>(curveIndex);
				return 0xFF00FFFF;  // 紫色
			}

			
			int EditPoint(size_t curveIndex, int pointIndex, ImVec2 value) override {
				static_cast<void>(curveIndex);
				mPoints[pointIndex] = value;
				return pointIndex;
			}
			void AddPoint(size_t curveIndex, ImVec2 value) override {
				static_cast<void>(curveIndex);
				static_cast<void>(value);
			}
			// DelPoint の第2引数も int に変更
			// void DelPoint(size_t curveIndex, size_t pointIndex) override {}
			// 存在しない可能性あり
		};

		CurveDelegate m_CurveDelegate;
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
		// 初期設定
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// ディタ全体ウィンドウ開始
		ImGui::Begin("Animation Editor");
		
		

		WindowManager();
		
		ModelOperate();

		ImGui::End();

		// 量産されたサブウィンドウたちの描画ループ
		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {

			if (!impl->m_SubWindows[i].is_open) continue;

			// 参照をとりだす
			auto& window = impl->m_SubWindows[i];

			std::string window_title = "アニメーション編集: " +
				window.name + "##" + std::to_string(window.id);

			if (window.request_focus) {
				ImGui::SetNextWindowFocus();
				window.request_focus = false;
			}

			ImGui::SetNextWindowPos(ImVec2(500.0f, 500.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(350.0f, 200.0f), ImGuiCond_FirstUseEver);


			// サブウィンドウの生成
			ImGui::Begin(window_title.c_str(), &window.is_open);
			ImGui::Text("Window ID: %d", window.id);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// ★【1番】このウィンドウ固有のタイムライン設定を描画！
			// 全体の長さ
			if (ImGui::InputInt("Max Frame", &window.maxFrame)) {
				if (window.maxFrame < 1) window.maxFrame = 1;
			}
			if (window.currentFrame > window.maxFrame) {
				window.currentFrame = window.maxFrame;
			}
			// 現在のフレーム位置（自分の maxFrame を最大値にする）
			ImGui::SliderInt("Current Frame", &window.currentFrame, 0, window.maxFrame);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// キーフレーム追加UI
			if (ImGui::Button("Insert Keyframe")) {
				// すでに同じフレームにキーが存在するかチェック
				auto it = std::find_if(window.keyFrames.begin(), window.keyFrames.end(),
					[&](const Impl::WindowData::KeyFrame& k) { return k.frame == window.currentFrame; });

				if (it != window.keyFrames.end()) {
					// すでに同じフレームにキーがあれば、現在の座標で上書き（今はまだ仮で0リセット）
					it->value = Vector3{ 0.0f, 0.0f, 0.0f };
				} else {
					// 新しいフレームなら、新規追加
					Impl::WindowData::KeyFrame newKey;
					newKey.frame = window.currentFrame;
					newKey.value = Vector3{ 0.0f, 0.0f, 0.0f }; // 今はまだ仮の値
					newKey.easing = EasingType::Lerp;

					window.keyFrames.push_back(newKey);

					// フレーム順（昇順）に並び替え
					std::sort(window.keyFrames.begin(), window.keyFrames.end(),
						[](const Impl::WindowData::KeyFrame& a, const Impl::WindowData::KeyFrame& b) {
							return a.frame < b.frame;
						});
				}
			}
			// 登録されてるキーフレームの一覧
			ImGui::Text("キーフレーム数: %d", static_cast<int>(window.keyFrames.size()));
			for (size_t k = 0; k < window.keyFrames.size(); ++k) {
				ImGui::Text("  [%d] Frame: %d", static_cast<int>(k), window.keyFrames[k].frame);
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
			impl->m_SubWindows.push_back({ allocated_id, name, true });

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
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// Animation Editor ウィンドウ生成
		ImGui::Begin("Edit");
		{
			// ウィンドウいっぱいにグラフを表示させるサイズ指定
			ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x < 100.0f) size.x = 100.0f;
            if (size.y < 200.0f) size.y = 200.0f;

            ImCurveEdit::Edit(impl->m_CurveDelegate, size, 1);
		}
		ImGui::End();
	}

	void AnimEdit::SaveSettings(const char* filePath) {
		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// フォルダ階層がない場合自動作成
		std::filesystem::path p(filePath);
		if (p.has_parent_path()) {
			std::filesystem::create_directories(p.parent_path());
		}

		json j = json::array();
		for (const auto& w : impl->m_SubWindows) {
			json window_json;
			window_json["id"] = w.id;
			window_json["name"] = w.name;
			window_json["is_open"] = false;

			// ★ セーブデータに項目を追加
			window_json["max_frame"] = w.maxFrame;
			window_json["current_frame"] = w.currentFrame;

			j.push_back(window_json);
		}

		std::ofstream file(filePath);
		if (file.is_open()) {
			file << j.dump(4); // インデント4スペースで綺麗に出力

			// セーブ成功
			std::string successMes = "[Animation Editor]\nSave Successed.\nPath: " + std::string(filePath);
			Logger::LogSuccess(successMes);
		} else {
			// セーブ失敗
			std::string failMes = "[Animation Editor]\nSave failed.\nFailed Path: " + std::string(filePath);
			Logger::LogWarning(failMes);
		}
	}

	void AnimEdit::LoadSettings(const char* filePath) {
		std::ifstream file(filePath);
		if (!file.is_open()) {
			std::string failMes = "[Animation Editor]\nLoad failed. File could not be opened.\nPath: " + std::string(filePath);
			Logger::LogWarning(failMes);
			return; // ファイルがなければ何もしない
		}

		json j;
		try {
			file >> j;
		}
		catch (const json::parse_error& e) {
			// 【JSONのパースに失敗した場合】
			std::string failMes = "[Animation Editor]\nLoad failed. JSON Parse Error: " + std::string(e.what()) + "\nPath: " + std::string(filePath);
			Logger::LogWarning(failMes);
			return; // JSONのパースエラー時は安全のため中断
		}

		AnimEdit& instance = GetInstance();
		Impl* impl = instance.m_pImpl;

		// 既存のリストをクリアして上書き
		impl->m_SubWindows.clear();
		impl->m_SelectedWindowIdx = -1;

		if (j.is_array()) {
			for (const auto& item : j) {
				if (item.contains("id") && item.contains("name")) {
					Impl::WindowData w;
					w.id = item["id"].get<int32_t>();
					w.name = item["name"].get<std::string>();
					w.is_open = false;
					w.request_focus = false;

					// ★ ロード処理を追加（古いセーブデータでキーがない場合の安全ガード付き）
					w.maxFrame = item.value("max_frame", 60);
					w.currentFrame = item.value("current_frame", 0);

					impl->m_SubWindows.push_back(w);
				}
			}
		}
		// 【ロード成功】
	    // すべての処理が正常に終わったら成功ログを出す
		std::string successMes = "[Animation Editor]\nLoad Successed.\nPath: " + std::string(filePath);
		Logger::LogSuccess(successMes);
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
