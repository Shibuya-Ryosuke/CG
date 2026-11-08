#include "Test.h"
#include "TestSceneFactory.h"

using namespace RyoEngine;

namespace Test {
	void Test::Initialize() {
		// ImGui で調整した値の保存先を設定して、読み込む
		const std::string jsonFolder = "Resources/EngineResources/Test/Json/";
		ParamEditor::SetFolderPath(jsonFolder);
		LightManager::SetFolderPath(jsonFolder);
		GPUParticleManager::SetFolderPath(jsonFolder);
		ParamEditor::Load();
		LightManager::Load();
		GPUParticleManager::Load();

		// シーン工場を渡して、最初のシーンを予約する (次の SceneManager::Update で作られる)
		SceneManager::SetSceneFactory(std::make_unique<TestSceneFactory>());
		SceneManager::ChangeScene("TEST");
	}

	void Test::Finalize() {
		// このアプリで作ったカメラを消す (CameraManager は静的クラスなので残さない)
		CameraManager::Clear();
	}
}
