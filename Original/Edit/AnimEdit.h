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

#ifdef _DEBUG
		static void Initialize();
		static void Update();

		static void WindowManager();
		static void DrawUI();

		static void SaveSettings(const char* filePath = "resources/json/editor/animationEditor.json");
		static void LoadSettings(const char* filePath = "resources/json/editor/animationEditor.json");

		static void ModelOperate();
		static void SetTargetModel(Model* model, const std::string& name);

		// ★ 追加：外部のboolフラグを名前付きでエディタに登録する関数
		static void RegisterTriggerFlag(const std::string& name, bool* ptr);
#else
		static void Initialize(){}
		static void Update(){}
		static void WindowManager();
		static void DrawUI(){}

		static void SaveSettings(const char* filePath = "resources/json/editor/animationEditor.json") { (void)filePath; }
		static void LoadSettings(const char* filePath = "resources/json/editor/animationEditor.json") { (void)filePath; }

		static void ModelOperate() {};

		static void SetTargetModel(Model* model, const std::string& name) { (void)model; (void)name; }

		// リリースビルド時は何もしない
		static void RegisterTriggerFlag(const std::string& name, bool* ptr) { (void)name; (void)ptr; }
#endif


	private:
		static AnimEdit& GetInstance() {
			static AnimEdit instance;
			return instance;
		}

		AnimEdit();
		~AnimEdit();

#ifdef _DEBUG
		struct Impl;
		Impl* m_pImpl = nullptr;
#endif
	};
}