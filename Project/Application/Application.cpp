#include "Application.h"
#include "Scene/SceneSetup.h"

using namespace RyoEngine;

namespace App {
	void Application::Initialize() {
		// ImGui で調整した値の保存先を設定して、読み込む
		const std::string jsonFolder = "Resources/ApplicationResources/Json/";
		ParamEditor::SetFolderPath(jsonFolder);
		LightManager::SetFolderPath(jsonFolder);
		ParticleManager::SetFolderPath(jsonFolder);
		ParamEditor::Load();
		LightManager::Load();
		// パーティクル (JSONが無ければテンプレから見本を作る)
		ParticleManager::LoadOrCreateDefaults();

		// シーンの登録と、起動時のシーンの指定 (Scene/SceneSetup.cpp)
		SetupScenes();
	}

	void Application::Finalize() {
	}
}
