#include "GameName.h"
#include "GameResources/GameTex.h"
#include "GameResources/GameSound.h"

using namespace RyoEngine;

namespace Game1 {
	void Game1::Initialize(RyoEngine::DebugCamera& debugCamera) {
		// リソース初期化
		GameImage::Initialize();
		GameSound::Initialize();

		// ゲーム初期化
		sceneManager_ = std::make_unique<SceneManager>();
		sceneManager_->Initialize(debugCamera, Scene::Title);
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