#include "Player.h"

using namespace RyoEngine;

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize() {
	// モデルの生成
	model_ = Model::Create("resources/RailSTG/TR.obj");
	model_->SetTranslate({ 0.0f,0.0f,0.0f });
}

void Player::Finalize() {

}

void Player::Update(const RyoEngine::Camera& camera) {
	model_->Update(camera);
}

void Player::Draw() {
	model_->Draw();
}