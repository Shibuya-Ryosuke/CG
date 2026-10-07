#include "AL4_1.h"
#include "GameResources/GameTex.h"
#include "GameResources/GameSound.h"

using namespace RyoEngine;

namespace AL4_1 {
	void AL4_1::Initialize() {
		// リソース初期化
		GameImage::Initialize();
		GameSound::Initialize();

		// ゲーム初期化
		sceneManager_ = std::make_unique<SceneManager>();
		sceneManager_->Initialize(Scene::Game);

		ParamEditor::SetFolderPath("Resources/ApplicationResources/AL4-1/Json/");
		LightManager::SetFolderPath("Resources/ApplicationResources/AL4-1/Json/");
		ParamEditor::Load();
		LightManager::Load();
	}
	void AL4_1::Update() {
		sceneManager_->Update();

		
		PrimitiveRenderer::SetCamera(GetActiveCamera(sceneManager_->GetCamera()));
	}
	void AL4_1::Draw() {
		sceneManager_->Draw(GetActiveCamera(sceneManager_->GetCamera()));
	}
	void AL4_1::Finalize() {
		sceneManager_->Finalize();
	}
}