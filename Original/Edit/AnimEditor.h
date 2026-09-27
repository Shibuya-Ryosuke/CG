#pragma once
#include <string>

namespace RyoEngine {

	class Model;
	class Camera;

	class AnimEditor {
	public:

		// コピーと代入を禁止
		AnimEditor(const AnimEditor&) = delete;
		AnimEditor& operator=(const AnimEditor&) = delete;
		AnimEditor(AnimEditor&&) = delete;
		AnimEditor& operator=(AnimEditor&&) = delete;

		// ★変更：以下は「実行時コア機能」（トリガー判定・SRT適用・セーブ/ロード・
		// モデル/フラグ登録）。ImGuiのエディタUIに依存しないため、
		// デバッグ／リリースどちらのビルドでも実際に動作する。
		static void Initialize();
		static void Update();

		static void SaveSettings(const char* filePath = "resources/Json/Editor/animationEditor.json");
		static void LoadSettings(const char* filePath = "resources/Json/Editor/animationEditor.json");

		// モデルを登録
		static void SetTargetModel(Model* model, const std::string& name);

		// ★追加：カメラを登録（Model同様、Translate/Rotateに加えFovY(ズーム)も編集対象にできる）
		static void SetTargetCamera(Camera* camera, const std::string& name);

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
		static AnimEditor& GetInstance() {
			static AnimEditor instance;
			return instance;
		}

		AnimEditor();
		~AnimEditor();

		struct Impl;
		Impl* m_pImpl = nullptr;
	};
}