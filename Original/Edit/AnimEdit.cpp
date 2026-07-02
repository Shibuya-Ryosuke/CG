#include "AnimEdit.h"
#include "../Base/Logger.h"
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
		};

		std::vector<WindowData> m_SubWindows;
		int32_t m_SelectedWindowIdx = -1;


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
		

		ImGui::End();

		for (size_t i = 0; i < impl->m_SubWindows.size(); i++) {

			// 【変更点2】×ボタンが押された時は、消去せず「非表示（スキップ）」にするだけに修正
			if (!impl->m_SubWindows[i].is_open) continue;

			std::string window_title = "新規ウィンドウ " +
				std::to_string(impl->m_SubWindows[i].id) + "##" + std::to_string(impl->m_SubWindows[i].id);

			if (impl->m_SubWindows[i].request_focus)
			{
				ImGui::SetNextWindowFocus();
				impl->m_SubWindows[i].request_focus = false;
			}

			ImGui::SetNextWindowPos(ImVec2(500.0f, 500.0f), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(300.0f, 200.0f), ImGuiCond_FirstUseEver);

			ImGui::Begin(window_title.c_str(), &impl->m_SubWindows[i].is_open);
			ImGui::Text("Window ID: %d", impl->m_SubWindows[i].id);
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
			// 要望通り、次回ロード時は閉じたいので false を保存（またはロード側で強制制御）
			window_json["is_open"] = false;
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
					// 「とりあえず閉じた状態にしておいて」のご要望通り、一律 false に設定
					w.is_open = false;
					w.request_focus = false;
					impl->m_SubWindows.push_back(w);
				}
			}
		}
		// 【ロード成功】
	    // すべての処理が正常に終わったら成功ログを出す
		std::string successMes = "[Animation Editor]\nLoad Successed.\nPath: " + std::string(filePath);
		Logger::LogSuccess(successMes);
	}
}

#else
namespace RyoEngine {
	AnimEdit::AnimEdit() = default;
	AnimEdit::~AnimEdit() = default;
}
#endif
