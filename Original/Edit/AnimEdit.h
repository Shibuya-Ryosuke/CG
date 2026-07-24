#pragma once
#include <string>

namespace RyoEngine {

	class Model;

	class AnimEdit {
	public:

		// コピーと代入を禁止
		AnimEdit(const AnimEdit&) = delete;
		AnimEdit& operator=(const AnimEdit&) = delete;
		AnimEdit(AnimEdit&&) = delete;
		AnimEdit& operator=(AnimEdit&&) = delete;

		// ★変更：以下は「実行時コア機能」（トリガー判定・SRT適用・セーブ/ロード・
		// モデル/フラグ登録）。ImGuiのエディタUIに依存しないため、
		// デバッグ／リリースどちらのビルドでも実際に動作する。
		static void Initialize();
		static void Update();

		static void SaveSettings(const char* filePath = "resources/json/editor/animationEditor.json");
		static void LoadSettings(const char* filePath = "resources/json/editor/animationEditor.json");

		// モデルを登録
		static void SetTargetModel(Model* model, const std::string& name);

		// フラグを登録
		static void RegisterFlag(const std::string& name, bool* ptr);

		// ★以下はエディタUI専用（ImGuiに依存）。デバッグビルドでのみ実体を持つ。
		// リリースビルドでは何もしない関数として扱われる。
#ifdef _DEBUG
		static void WindowManager();
		static void DrawUI();
		static void ModelOperate();
#else
		static void WindowManager() {}
		static void DrawUI() {}
		static void ModelOperate() {}
#endif

	private:
		static AnimEdit& GetInstance() {
			static AnimEdit instance;
			return instance;
		}

		AnimEdit();
		~AnimEdit();

		struct Impl;
		Impl* m_pImpl = nullptr;
	};
}