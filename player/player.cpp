#include "player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/mirror.obj");
	model_->SetTex("resources/flower.png");

	AnimEdit::RegisterTriggerFlag("player : pushA", &pushA_);
	AnimEdit::RegisterTriggerFlag("player : playManager", &playManager_);
}

void Player::Update(RyoEngine::DebugCamera& debugCamera) {
	if(Input::TriggerKey(DIK_A)) {
		pushA_ = !pushA_;
	}
	playManager_ = true;
	if (Input::PushKey(DIK_M)) {
		playManager_ = false;
	}

	model_->Update(debugCamera);
}

void Player::Draw() {
	model_->Draw();
}