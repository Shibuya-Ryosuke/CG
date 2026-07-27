#include "EnemyBullet.h"

using namespace RyoEngine;

EnemyBullet::~EnemyBullet() {
	delete model_;
	model_ = nullptr;
}

void EnemyBullet::Initialize(const Vector3& position, const Vector3& velocity) {
	model_ = Model::Create("resources/AL3-03/enemyBullet/enemyBullet.obj");
	model_->SetTex("resources/AL3-03/enemyBullet/enemyBullet.png");
	model_->SetColor({ 0.0f,0.0f,0.0f,1.0f });

	model_->SetTranslate(position);
	velocity_ = velocity;
}

void EnemyBullet::Update(const RyoEngine::Camera& camera) {
	model_->SetTranslate(model_->GetTranslate() + velocity_);
	model_->Update(camera);

	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}
}

void EnemyBullet::Update(const RyoEngine::DebugCamera& debugCamera) {
	// 座標を移動させる
	model_->SetTranslate(model_->GetTranslate() + velocity_);
	// 更新
	model_->Update(debugCamera);

	// 時間経過でデス
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}
}

void EnemyBullet::Draw() {
	model_->Draw();
}