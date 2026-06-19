#include "GameScene.h"

using namespace RyoEngine;


GameScene::GameScene(){}
GameScene::~GameScene() {
	delete player_;
}

void GameScene::Initialize() {
	// 自キャラの生成
	player_ = new Player();
	player_->Initialize();

}

void GameScene::Update() {
	// 自キャラの更新
	player_->Update();
}

void GameScene::Draw() {
	Begin3dDraw();
	player_->Draw();










	Begin2dDraw();

}
