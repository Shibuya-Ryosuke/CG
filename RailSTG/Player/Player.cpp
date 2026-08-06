#include "Player.h"

using namespace RyoEngine;

void Player::Initialize() {

}

void Player::Finalize() {

}

void Player::Update(RyoEngine::Camera& camera) {
	model_->Update(camera);
}

void Player::Draw() {
	model_->Draw();
}