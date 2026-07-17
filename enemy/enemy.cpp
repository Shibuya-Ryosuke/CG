#include "enemy.h"
using namespace RyoEngine;

void Enemy::Initialize() {
	model_->SetTex("resources/wall.png");
	model_->SetTranslate({ 2.0f,0.0f,0.0f });
	model_->SetRotate({ 0.0f,0.0f,0.0f });

	isAlive_ = true;

	AnimEdit::RegisterFlag("enemy : isAlive", &isAlive_);
}

void Enemy::Update(RyoEngine::DebugCamera& debugCamera) {
	model_->Update(debugCamera);
}

void Enemy::Draw() {
	model_->Draw();
}