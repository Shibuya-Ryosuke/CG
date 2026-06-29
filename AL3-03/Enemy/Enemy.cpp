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
	UpdatePhase();
	model_->Update(camera);
}

void Enemy::Update(RyoEngine::DebugCamera& debugCamera) {
	UpdatePhase();
	model_->Update(debugCamera);
}

void Enemy::Draw() {
	model_->Draw();
}

void Enemy::PhaseApproach() {
	// 接近フェーズ時のvelocity
	Vector3 approachVelocity = velocity_ * kApproachSpeedRate_;
	// 移動
	model_->SetTranslate(model_->GetTranslate() + approachVelocity);
	// 既定の位置に到達したら離脱
	if (model_->GetTranslate().z < 0.0f) {
		phase_ = Phase::Leave;
	}
}

void Enemy::PhaseLeave() {
	// 離脱フェーズ時のvelocity
	Vector3 leaveVelocity{
		.x = -kMoveSpeed_,
		.y = kMoveSpeed_,
		.z = -kMoveSpeed_,
	};
	leaveVelocity *= kLeaveSpeedRate_;
	// 移動
	model_->SetTranslate(model_->GetTranslate() + leaveVelocity);
}

void Enemy::UpdatePhase() {
	switch (phase_) {
	case Phase::Approach:
		PhaseApproach();
		break;

	case Phase::Leave:
		PhaseLeave();
		break;
	}
}