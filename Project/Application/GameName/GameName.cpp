#include "GameName.h"
#include "GameResources/GameTex.h"
#include "GameResources/GameSound.h"

using namespace RyoEngine;

namespace Game1 {
	void Game1::Initialize() {
		// リソース初期化
		GameImage::Initialize();
		GameSound::Initialize();

		// ゲーム初期化
		sceneManager_ = std::make_unique<SceneManager>();
		sceneManager_->Initialize(Scene::Title);

		ParamEditor::SetFolderPath("Resources/ApplicationResources/Game1/Json/");
		LightManager::SetFolderPath("Resources/ApplicationResources/Game1/Json/");
		ParamEditor::Load();
		LightManager::Load();
	}
	void Game1::Update() {
		sceneManager_->Update();
	}
	void Game1::Draw() {
		sceneManager_->Draw();
	}
	void Game1::Finalize() {
		sceneManager_->Finalize();
	}
}