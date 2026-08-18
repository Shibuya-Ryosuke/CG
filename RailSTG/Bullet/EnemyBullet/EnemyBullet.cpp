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
	if (hp_ <= 0.0f)return;

	// ターゲットが居るとき
	if (hasTarget_) {
		// 自分の現在位置からターゲットへの方向を毎フレーム再計算
		Vector3 toTarget = targetPos_ - GetTranslate();
		Vector3 direction = Normalize(toTarget);

		if (isDeflected_) {
			velocity_ = direction * kDeflectedSpeed;
		} else {
			float speed = Length(velocity_);
			velocity_ = direction * speed;
		}
		

		// デバッグ用に色を赤
		// 跳ね返されたときだけ色の変更
		if (isDeflected_) {
			model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });
		}
	}

	// 座標の取得
	Vector3 translate = model_->GetTranslate();

	// 移動
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// obb
	UpdateOBB(obb_, model_.get());
}

void EnemyBullet::Draw() {
	BaseBullet::Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
}