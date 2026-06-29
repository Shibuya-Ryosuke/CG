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

// staticで宣言したメンバ関数ポインタテーブルの実体
void (Enemy::* Enemy::spFuncTable[])() = {
	&Enemy::PhaseApproach,  // 要素番号0
	&Enemy::PhaseLeave      // 要素番号1
};

void Enemy::Update(RyoEngine::Camera& camera) {
	(this->*spFuncTable[static_cast<size_t>(phase_)])();
	model_->Update(camera);
}

void Enemy::Update(RyoEngine::DebugCamera& debugCamera) {
	(this->*spFuncTable[static_cast<size_t>(phase_)])();
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