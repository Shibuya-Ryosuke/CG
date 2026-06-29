#include "GameScene.h"

using namespace RyoEngine;


GameScene::GameScene(){}
GameScene::~GameScene() {
	delete player_;
	delete enemy_;
	delete axisIndicator_;
	delete debugCamera_;
	delete camera_;
}

void GameScene::Initialize() {
	// 自キャラの生成
	player_ = new Player();
	player_->Initialize();
	// 敵の生成
	enemy_ = new Enemy();
	enemy_->Initialize({0,0,30},{0,0,-enemy_->GetMoveSpeed()});

	// 軸生成
	axisIndicator_ = new AxisIndicator();
	axisIndicator_->ToggleVisible();

	debugCamera_ = new DebugCamera();
	debugCamera_->ToggleIsAvailable();
	debugCamera_->SetTranslate({ debugCamera_->GetTranslate().x,debugCamera_->GetTranslate().y, -80.0f });

	camera_ = new Camera();
	camera_->SetTranslate({ camera_->GetTranslate().x,camera_->GetTranslate().y, -80.0f });
}

void GameScene::Update() {
	if (Input::TriggerKey(DIK_Z)) {
		axisIndicator_->ToggleVisible();
	}

	if (!debugCamera_->GetIsAvailable()) {
		if (Input::TriggerKey(DIK_C)) {
			debugCamera_->ToggleIsAvailable();
		}
		camera_->Update();
		axisIndicator_->Update(*camera_);

		// 自キャラの更新
		player_->Update(*camera_);
		// 敵の更新
		if (enemy_) {
			enemy_->Update(*camera_);
		}
	} else {
		if (Input::TriggerKey(DIK_C)) {
			debugCamera_->ToggleIsAvailable();
		}
		debugCamera_->Update();
		axisIndicator_->Update(*debugCamera_);


		player_->Update(*debugCamera_);
		
		if (enemy_) {
			enemy_->Update(*debugCamera_);
		}
	}
	
}

void GameScene::Draw() {
	Begin3dDraw();
	axisIndicator_->Draw();
	player_->Draw();
	if (enemy_) {
		enemy_->Draw();
	}










	Begin2dDraw();

}
