#include "player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/mirror.obj");
	model_->SetTex("resources/flower.png");

	AnimEdit::RegisterTriggerFlag("player : toggleA", &toggleA_);
	AnimEdit::RegisterTriggerFlag("player : holdM", &hold_M_);
}

void Player::Update(RyoEngine::DebugCamera& debugCamera) {
	if(Input::TriggerKey(DIK_A)) {
		toggleA_ = !toggleA_;
	}
	hold_M_ = true;
	if (Input::PushKey(DIK_M)) {
		hold_M_ = false;
	}

	model_->Update(debugCamera);
}

void Player::Draw() {
	model_->Draw();
}