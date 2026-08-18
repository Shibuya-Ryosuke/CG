#include "EnemyBullet.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

EnemyBullet::EnemyBullet() = default;

void EnemyBullet::Initialize() {
	if (model_ == nullptr) {
		// 生成
		model_ = Model::Create("resources/RailSTG/Bullet/bullet.obj");
		model_->SetTex("resources/RailSTG/Bullet/flower.png");
		// お試しで小さく
		model_->SetScale({ 0.5f,0.5f,0.5f });
	}
}

void EnemyBullet::Finalize() {

}

void EnemyBullet::Update(const RyoEngine::Camera& camera) {
	// 寿命の減少
	BaseBullet::Update(camera);
	if (isDead_)return;

	// 跳ね返されたかつターゲットが居るとき
	if (isDeflected_ && hasTarget_) {
		// 自分の現在位置からターゲットへの方向を毎フレーム再計算
		Vector3 toTarget = targetPosition_ - GetTranslate();
		Vector3 direction = Normalize(toTarget);

		// 速度の「大きさ」は反射時に決めた値(1.5倍)を維持し、向きだけ更新
		velocity_ = direction * (baseSpeed_ * deflectedSpeedScale_);

		// デバッグ用に色を赤
		model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });
	}

	// 座標の取得
	Vector3 translate = model_->GetTranslate();

	// 移動
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// obb
	UpdateOBB(obb_, model_.get(), { 0.5f,0.5f,0.5f });
}

void EnemyBullet::Draw() {
	BaseBullet::Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
}