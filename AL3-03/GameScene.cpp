#include "GameScene.h"

using namespace RyoEngine;


GameScene::GameScene(){}
GameScene::~GameScene() {
	delete player_;
	delete debugCamera_;
	delete camera_;
}

void GameScene::Initialize() {
	// 自キャラの生成
	player_ = new Player();
	player_->Initialize();

	debugCamera_ = new DebugCamera();
	camera_ = new Camera();
}

void GameScene::Update() {
	if (!debugCamera_->GetIsAvailable()) {
		if (Input::TriggerKey(DIK_SPACE)) {
			debugCamera_->ToggleIsAvailable();
		}
		camera_->Update();

		// 自キャラの更新
		player_->Update(*camera_);

	} else {
		if (Input::TriggerKey(DIK_SPACE)) {
			debugCamera_->ToggleIsAvailable();
		}
		debugCamera_->Update();

		// 自キャラの更新
		player_->Update(*debugCamera_);
	}
	
}

void GameScene::Draw() {
	Begin3dDraw();
	player_->Draw();










	Begin2dDraw();

}
