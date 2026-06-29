#include "Enemy.h"

using namespace RyoEngine;

Enemy::~Enemy() {
	delete model_;
	model_ = nullptr;
}

void Enemy::Initialize(const Vector3& position, const Vector3& velocity) {
	model_ = Model::Create("resources/AL3-03/enemy/enemy.obj");
	model_->SetTex("resources/AL3-03/enemy/enemy.png");

	model_->SetTranslate(position);
	velocity_ = velocity;
}

void Enemy::Update(RyoEngine::Camera& camera) {
	model_->SetTranslate(model_->GetTranslate() + velocity_);
	model_->Update(camera);
}

void Enemy::Update(RyoEngine::DebugCamera& debugCamera) {
	model_->SetTranslate(model_->GetTranslate() + velocity_);
	model_->Update(debugCamera);
}

void Enemy::Draw() {
	model_->Draw();
}
