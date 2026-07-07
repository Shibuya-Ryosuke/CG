#include "player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/mirror.obj");
	model_->SetTex("resources/flower.png");

	AnimEdit::RegisterTriggerFlag("player : pushA", &pushA_);
}

void Player::Update(RyoEngine::DebugCamera& debugCamera) {
	if(Input::TriggerKey(DIK_A)) {
		pushA_ = !pushA_;
	}
	model_->Update(debugCamera);
}

void Player::Draw() {
	model_->Draw();
}