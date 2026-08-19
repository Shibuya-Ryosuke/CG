#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <memory>
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"
#include "../Time/TimeManager.h"
#include "EnemyEnum.h"

class EnemyBullet;

class BaseEnemy : public BaseObject {
public:
	BaseEnemy() = default;
	~BaseEnemy() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	BaseEnemy(const BaseEnemy&) = delete;
	BaseEnemy& operator=(const BaseEnemy&) = delete;

	RyoEngine::Vector3 GetTranslate() const { return model_->GetTranslate(); }
	const RyoEngine::OBB& GetOBB() const { return obb_; }

	void SetTranslate(const RyoEngine::Vector3 translate) { model_->SetTranslate(translate); }
	void SetTranslateY(const float y) { model_->SetTranslateY(y); }

	void SetFollowOffset(const RyoEngine::Vector3& offset) { followOffset_ = offset; }

	void SetVelocity(const RyoEngine::Vector3 velocity) { velocity_ = velocity; }
	void SetVelocityZ(const float velocityZ) { velocity_.z = velocityZ; }

	float GetHp() const { return hp_; }
	void SetHp(float hp) { hp_ = hp; }

	bool IsDead() const { return isDead_; }
	void SetIsDead(bool isDead) { isDead_ = isDead; }

	LockOnState GetLockOnState() { return lockOnState_; }
	void SetLockOnState(LockOnState state) { lockOnState_ = state; }

	int32_t GetEnemyId() const { return enemyId_; }

	// 弾の取得関数
	virtual const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const = 0;

	void DrawLockOnEffect() {
		switch (lockOnState_) {
		case LockOnState::Hoverd:
			RyoEngine::PrimitiveRenderer::DrawSphere(model_->GetWorldPos(), 1.0f, 16, { 0.0f,1.0f,0.0f,1.0f }, RyoEngine::PrimitiveDrawMode::Fill);
			break;

		case LockOnState::Locked:
			RyoEngine::PrimitiveRenderer::DrawSphere(model_->GetWorldPos(), 1.0f, 16, { 1.0f,0.0f,0.0f,1.0f }, RyoEngine::PrimitiveDrawMode::Fill);
			break;

		case LockOnState::None:
		default:
			break;
		}
	}

	void OnCollision(float damage) {
		hp_ -= damage;
		if (hp_ <= 0.0f) {
			isDead_ = true;
		}
	}

	void SpawnAnimation() {
		if (isSpawning_) {
			spawnAnimTimer_ += TimeManager::GetDeltaTime();
			float t = spawnAnimTimer_ / kSpawnAnimDuration_; // 1.0秒で正規化 (0.0 ～ 1.0)

			if (t >= 1.0f) {
				t = 1.0f;
				isSpawning_ = false; // 演出終了
			}

			// 1. スケール用イージング（はじめ遅く終わり早く ＝ easeIn など。例えば t * t）
			float easeScale = RyoEngine::EaseInQuart(t, 0.0f, 1.0f);
			// スケールを 0 から 1 へ
			model_->SetScale({ easeScale, easeScale, easeScale });

			// 2. 回転させる
			float currentRotationY = RyoEngine::EaseOutQuad(t, 0.0f, 10.0f * 2.0f * static_cast<float>(M_PI));
			model_->SetRotateY(currentRotationY); // Ｙ軸回転の場合の例
		}
	}

protected:
	// ID
	int32_t enemyId_ = -1;

	// 次に発行するID
	static inline int32_t nextEnemyId_ = 0;

	// 速度
	RyoEngine::Vector3 velocity_{};

	// hp
	float hp_ = 0.0f;

	// 死亡
	bool isDead_ = false;

	// 衝突判定用
	RyoEngine::OBB obb_{};
	RyoEngine::Vector3 baseObbSize_{};

	// ロックオンステート
	LockOnState lockOnState_ = LockOnState::None;

	// カメラ追従オフセット
	RyoEngine::Vector3 followOffset_{};

	// スポーンアニメーション
	float spawnAnimTimer_ = 0.0f;
	bool isSpawning_ = true;
	float kSpawnAnimDuration_ = 1.5f;
};