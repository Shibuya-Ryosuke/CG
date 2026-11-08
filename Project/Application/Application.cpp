#include "Application.h"
#include "Scene/SceneFactory.h"

using namespace RyoEngine;

namespace App {
	void Application::Initialize() {
		// ImGui で調整した値の保存先を設定して、読み込む
		const std::string jsonFolder = "Resources/ApplicationResources/Json/";
		ParamEditor::SetFolderPath(jsonFolder);
		LightManager::SetFolderPath(jsonFolder);
		GPUParticleManager::SetFolderPath(jsonFolder);
		ParamEditor::Load();
		LightManager::Load();
		GPUParticleManager::Load();

		// シーン工場を渡して、最初のシーンを予約する
		SceneManager::SetSceneFactory(std::make_unique<SceneFactory>());
		SceneManager::ChangeScene("Title");
	}

	void Application::Finalize() {
	}
}
