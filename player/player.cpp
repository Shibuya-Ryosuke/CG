#include "player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/mirror.obj");
	model_->SetTex("resources/flower.png");

	AnimEdit::RegisterTriggerFlag("player : pushA", &pushA_);
	AnimEdit::RegisterTriggerFlag("player : roop", &isRoop_);
}

void Player::Update(RyoEngine::DebugCamera& debugCamera) {
	if(Input::TriggerKey(DIK_A)) {
		pushA_ = !pushA_;
	}

	if (Input::TriggerKey(DIK_P)) {
		isRoop_ = !isRoop_;
	}

	model_->Update(debugCamera);
}

void Player::Draw() {
	model_->Draw();
}