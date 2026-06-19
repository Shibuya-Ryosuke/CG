#include "Player.h"

using namespace RyoEngine;

void Player::Initialize() {
	model_ = Model::Create("resources/AL3-03/player.obj");
	model_->SetTex("resources/flower.png");
}

void Player::Update() {
	model_->
}

void Player::Draw() {
	model_->Draw();
}
