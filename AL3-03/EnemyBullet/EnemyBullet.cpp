#include "EnemyBullet.h"

using namespace RyoEngine;

EnemyBullet::~EnemyBullet() {
	delete model_;
	model_ = nullptr;
}

void EnemyBullet::Initialize(const Vector3& position, const Vector3& velocity) {
	model_ = Model::Create("resources/AL3-03/enemyBullet/enemyBullet.obj");
	model_->SetTex("resources/AL3-03/enemyBullet/enemyBullet.png");
	model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });

	model_->SetTranslate(position);
	model_->SetScale({ 0.5f,0.5f,3.0f });

	velocity_ = velocity;

	// --- 進行方向を向かせる処理 ---
	// Y軸まわりの角度（水平方向の向き）
	float rotationY = std::atan2(velocity_.x, velocity_.z);

	// 横軸方向の長さ（XZ平面での移動成分の長さ）を求める
	float horizontalLength = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);

	// X軸まわりの角度（垂直方向の向き）
	float rotationX = std::atan2(-velocity_.y, horizontalLength);

	// モデルに回転を適用（※RyoEngineのモデルクラスにある関数名に合わせて調整してください）
	model_->SetRotate({ rotationX, rotationY, 0.0f });
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