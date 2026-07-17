#include "player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_->SetTex("resources/flower.png");
	model_->SetTranslate({ -4.0f,0.0f,0.0f });

	speed_ = 0.08f;
	isHit_ = false;
	isPlay_ = false;
	AnimEdit::RegisterFlag("player: hit", &isHit_);
	AnimEdit::RegisterFlag("player: play", &isPlay_);
}

void Player::Update(RyoEngine::DebugCamera& debugCamera) {
	if (!isHit_) {
		if (Input::PushKey(DIK_D)) {
			float currentPosX = model_->GetTranslate().x + speed_;
			model_->SetTranslate({ currentPosX,model_->GetTranslate().y,model_->GetTranslate().z});
		}

		if (Input::PushKey(DIK_A)) {
			float currentPosX = model_->GetTranslate().x - speed_;
			model_->SetTranslate({ currentPosX,model_->GetTranslate().y,model_->GetTranslate().z });
		}
	}
	if (Input::TriggerKey(DIK_SPACE)) {
		isPlay_ = true;
	}

	model_->Update(debugCamera);
}

void Player::Draw() {
	model_->Draw();
}