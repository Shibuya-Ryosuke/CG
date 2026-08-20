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

	RyoEngine::Vector3 GetFollowOffset() { return followOffset_; }
	void SetFollowOffset(const RyoEngine::Vector3& offset) { followOffset_ = offset; }

	RyoEngine::Vector3 GetVelocity() { return velocity_; }
	void SetVelocity(const RyoEngine::Vector3 velocity) { velocity_ = velocity; }
	void SetVelocityZ(const float velocityZ) { velocity_.z = velocityZ; }

	float GetHp() const { return hp_; }
	void SetHp(float hp) { hp_ = hp; }

	bool IsDead() const { return isDead_; }
	void SetIsDead(bool isDead) { isDead_ = isDead; }

	LockOnState GetLockOnState() { return lockOnState_; }
	void SetLockOnState(LockOnState state) { lockOnState_ = state; }

	int32_t GetEnemyId() const { return enemyId_; }

	bool IsDespawning() { return isDespawning_; }

	/// <summary>
	/// デスポーンの開始
	/// </summary>
	void DespawnStart() {
		isDespawning_ = true;
		animTimer_ = kAnimDuration_;
		// 停止
		velocity_ = { 0.0f,0.0f,0.0f };
	}

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

	bool OnCollision(float damage) {
		// 既に死亡しているなら無視
		if (isDead_) return false;

		// ダメージ処理
		hp_ -= damage;

		// hpが0以下なら死亡、このヒットで撃破されたことを返す
		if (hp_ <= 0.0f) {
			isDead_ = true;
			return true;
		}

		// そうでなければfalse
		return false;
	}

	void SpawnAnimation() {
		if (!isSpawning_)return;

		animTimer_ += TimeManager::GetDeltaTime();
		float t = animTimer_ / kAnimDuration_; // 1.0秒で正規化 (0.0 ～ 1.0)

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

	void DespawnAnimation() {
		if (!isDespawning_)return;

		// スケールを1から0にするのでタイムは減算させる
		animTimer_ -= TimeManager::GetDeltaTime();
		float t = animTimer_ / kAnimDuration_;

		if (t <= 0.0f) {
			t = 0.0f;
			isDespawning_ = false; // 演出終了
			isDead_ = true; // 死亡
		}

		// 1. スケール用イージング
		float easeScale = RyoEngine::EaseOutQuart(t, 0.0f, 1.0f);
		// スケールを 0 から 1 へ
		model_->SetScale({ easeScale, easeScale, easeScale });

		// 2. 回転させる
		float currentRotationY = RyoEngine::EaseOutQuad(t, 0.0f, 10.0f * 2.0f * static_cast<float>(M_PI));
		model_->SetRotateY(currentRotationY); // Ｙ軸回転の場合の例
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
	float animTimer_ = 0.0f;
	float kAnimDuration_ = 1.5f;
	bool isSpawning_ = true;
	bool isDespawning_ = false;
};