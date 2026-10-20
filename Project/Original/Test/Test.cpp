#include "Test.h"
#include "TestSceneFactory.h"

using namespace RyoEngine;

namespace Test {
	void Test::Test::Initialize() {
		ParamEditor::SetFolderPath("Resources/EngineResources/Test/Json/");
		LightManager::SetFolderPath("Resources/EngineResources/Test/Json/");
		GPUParticleManager::GetInstance()->SetFolderPath("Resources/EngineResources/Test/Json/");
		ParamEditor::Load();
		LightManager::Load();
		GPUParticleManager::GetInstance()->Load();

		// シーン工場をセットして、最初のシーンを予約する(次のSceneManager::Update()で生成・初期化される)
		SceneManager::SetSceneFactory(std::make_unique<TestSceneFactory>());
		SceneManager::ChangeScene("TEST");
	}
	void Test::Finalize() {
		// このゲームで作ったカメラを破棄する(CameraManagerは静的クラスなので、残さないように)
		CameraManager::Clear();
	}
}
