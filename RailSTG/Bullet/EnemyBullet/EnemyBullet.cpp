#include "EnemyBullet.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

EnemyBullet::EnemyBullet() = default;

void EnemyBullet::Initialize() {
	if (model_ == nullptr) {
		// 生成
		model_ = Model::Create("resources/RailSTG/Model/Bullet/bullet.obj");
		model_->SetTex("resources/RailSTG/Model/Bullet/enemyBullet_uv.png");
	}
	baseObbSize_ = { 0.8f,0.8f,0.8f };
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
			// 通常弾はturnRate_の分だけ少しずつ狙いを補正(HomingMissileと同じ考え方、弱め)
			float speed = Length(velocity_);
			Vector3 currentDir = Normalize(velocity_);
			Vector3 newDir = Normalize(currentDir + (direction - currentDir) * turnRate_);
			velocity_ = newDir * speed;
		}
		

		// デバッグ用に色を赤
		// 跳ね返されたときだけ色の変更
		if (isDeflected_) {
			model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });
		}

		// プレイヤーに対しての追従（至近距離で追尾打ち切り）
		if (!isDeflected_ && !isHomingDisabled_) {
			// ターゲットとの距離が kTargetDistance より近くなったら追尾をやめる
			Vector3 displacement = GetTranslate() - targetPos_;
			float distance = Length(displacement); // 2点間の距離を計算

			if (distance <= kTargetDistance) {
				hasTarget_ = false;
				isHomingDisabled_ = true;
			}
		}
	}

	// 座標の取得
	Vector3 translate = model_->GetTranslate();

	// 移動
	translate += velocity_ * TimeManager::GetDeltaTime();
	model_->SetTranslate(translate);

	model_->Update(camera);

	// obb
	UpdateOBB(obb_, baseObbSize_, model_.get());
}

void EnemyBullet::Draw() {
	BaseBullet::Draw();
	//PrimitiveRenderer::DrawOBB(obb_, { 1.0f,1.0f,1.0f,1.0f }, PrimitiveDrawMode::Wireframe);
}