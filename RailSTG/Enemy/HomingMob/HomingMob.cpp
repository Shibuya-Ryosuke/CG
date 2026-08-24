#ifdef _DEBUG
#include <imgui.h>
#endif

#include <algorithm>

#include "HomingMob.h"
#include "../BaseEnemy.h"
#include "../../Bullet/EnemyBullet/EnemyBullet.h"
#include "../../Time/TimeManager.h"
#include "../../GameMath/GameMath.h"

using namespace RyoEngine;

HomingMob::HomingMob() = default;
HomingMob::~HomingMob() = default;

void HomingMob::Initialize() {
	if (enemyMissileHandle_ == 0) {
		enemyMissileHandle_ = LoadTex("resources/RailSTG/Bullet/homingBullet_uv.png");
	}

	// 生成
	if (model_ == nullptr) {
		model_ = Model::Create("resources/RailSTG/Enemy/HomingMob/homingMob.obj");
	}
	// 速度セット
	SetVelocity(kVelocity);
	// hpセット
	SetHp(kMaxHp_);

	baseObbSize_ = { 1.0f,1.0f,1.0f };
	obb_.size = baseObbSize_;

	// ID格納
	enemyId_ = nextEnemyId_;
	// 次に入れるIDのために1増やす
	nextEnemyId_++;

	// 初期ステート
	state_ = MobState::Standard;
}

void HomingMob::Finalize() {}

void HomingMob::Update(const RyoEngine::Camera& camera) {

	// リクエストを反映
	if (request_ != MobState::None) {
		state_ = request_;
		request_ = MobState::None;
	}

	// 移動
	Move();

	// カメラに追従
	UpdateFollowTransform(model_.get(), camera, followOffset_);

	// アニメーション
	BaseEnemy::SpawnAnimation();
	BaseEnemy::DespawnAnimation();
	// 更新
	model_->Update(camera);
	// obb
	UpdateOBB(obb_, baseObbSize_, model_.get());

	if (followOffset_.z > 130.0f) {
		isDead_ = true;
	}

	
	// アニメーション中は攻撃しない
	if (isSpawning_)return;

	// 状態別処理
	switch (state_) {
	case MobState::Standard:
		// 射撃
		Shot();
		break;

	case MobState::None:
	default:
		break;
	}

	// 弾の更新
#ifdef _DEBUG
	ImGui::Begin("enemyBullets");
#endif
	for (auto& bullet : bullets_) {
		if (!bullet->IsDeflected()) {
			// 反射前は毎フレームプレイヤーを追い続ける
			bullet->SetTargetPos(targetPos_);
		}
		bullet->Update(camera);

#ifdef _DEBUG
		// 表示
		if (ImGui::TreeNodeEx("bullet", ImGuiTreeNodeFlags_DefaultOpen)) {
			Vector3 bulletT = bullet->GetTranslate();
			ImGui::Text("translate: (%.2f, %.2f, %.2f)", bulletT.x, bulletT.y, bulletT.z);
			Vector3 bulletV = bullet->GetVelocity();
			ImGui::Text("velocity : (%.2f, %.2f, %.2f)", bulletV.x, bulletV.y, bulletV.z);
			ImGui::Text("ownerId  : (%d)", bullet->GetOwnerId());
			ImGui::TreePop();
		}
#endif
	}
#ifdef _DEBUG
	ImGui::End();
#endif

	// 死んだ弾を削除 (erase-removeイディオム)
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<EnemyBullet>& b) { return b->IsDead(); }),
		bullets_.end()
	);
}

void HomingMob::Draw() {
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}
	model_->Draw();
	PrimitiveRenderer::DrawOBB(obb_, { 1.0f,0.0f,0.0f,1.0f }, PrimitiveDrawMode::Wireframe);
	// ロックオンエフェクト
	BaseEnemy::DrawLockOnEffect();
}

void HomingMob::Shot() {
	// メイン射撃のタイマー減少
	if (shotInterval_ > 0) {
		shotInterval_ -= TimeManager::GetDeltaTime();
	} else {
		// 0以下の時発射
		// 新しい弾作成
		auto bullet = std::make_unique<EnemyBullet>();
		bullet->Initialize();

		// モブの位置と向きを取得
		Vector3 position = model_->GetTranslate();
		Vector3 rotate = model_->GetRotate();

		// ローカルの正面(+Z)をモブの回転で変換 → ワールド空間の正面ベクトル
		Vector3 direction = targetPos_ - position;
		direction = Normalize(direction);

		// 弾に位置と速度をセット
		bullet->SetTranslate(position);
		bullet->SetVelocity(direction * kBulletSpeed);
		bullet->SetIsDeflectable(true);
		bullet->SetDamage(kBulletDamage);
		bullet->SetOwnerId(GetEnemyId());
		bullet->SetLifeTime(100.0f);
		bullet->SetScale({ 2.0f,2.0f,2.0f });
		// 撃ち落とし可能
		bullet->SetIsDestructible(true);
		// 体力の設定
		bullet->SetHp(kHomingBulletHp);
		// 発射直後からホーミング開始
		bullet->SetTargetPos(targetPos_);

		bullets_.push_back(std::move(bullet));

		// 発射間隔をリセット
		shotInterval_ = kShotInterval;
	}
}

void HomingMob::Move() {
	// 移動
	followOffset_ += velocity_ * TimeManager::GetDeltaTime();
}

void HomingMob::UpdateDeflectedBullets(const std::function<BaseEnemy* (int32_t)>& enemyFinder) {
	for (auto& bullet : bullets_) {
		if (bullet->IsDeflected()) {
			BaseEnemy* owner = enemyFinder(bullet->GetOwnerId());
			if (owner) {
				bullet->SetTargetPos(owner->GetWorldPos());
			} else {
				// 発射元が死亡済み → ターゲット解除(直進させる/自爆させる等は後で決める)
				bullet->ClearTarget();
			}
		}
	}
}
