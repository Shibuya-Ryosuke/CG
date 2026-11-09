#include "Test.h"
#include "TestScene.h"

using namespace RyoEngine;

namespace Test {
	void Test::Initialize() {
		// ImGui で調整した値の保存先を設定して、読み込む
		const std::string jsonFolder = "Resources/EngineResources/Test/Json/";
		ParamEditor::SetFolderPath(jsonFolder);
		LightManager::SetFolderPath(jsonFolder);
		ParticleManager::SetFolderPath(jsonFolder);
		ParamEditor::Load();
		LightManager::Load();
		ParticleManager::LoadOrCreateDefaults();

		// シーンを登録して、起動時のシーンにする (次の SceneManager::Update で作られる)
		SceneManager::RegisterScene<TestScene>();
		SceneManager::SetDefaultScene<TestScene>();
	}

	void Test::Finalize() {
		// このアプリで作ったカメラを消す (CameraManager は静的クラスなので残さない)
		CameraManager::Clear();
	}
}
