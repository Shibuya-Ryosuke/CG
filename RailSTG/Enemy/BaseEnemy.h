#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <memory>
#include "../../Original/RyoEngine.h"
#include "../BaseObject/BaseObject.h"
#include "../Time/TimeManager.h"
#include "../Particle/ParticleManager.h"
#include "EnemyEnum.h"
#include "../GameSound/GameSound.h"

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

	bool OnCollision(float damage) {
		// 既に死亡しているなら無視
		if (hp_ <= 0.0f) return false;
		GameSound::PlaySE(GameSound::SE::Hit);

		// ダメージ処理
		hp_ -= damage;
		damageTimer_ = kDamageTimer_;

		// hpが0以下なら死亡、このヒットで撃破されたことを返す
		if (hp_ <= 0.0f) {
			isDestroy_ = true;
			animTimer_ = kDestroyDuration_;
			GameSound::PlaySE(GameSound::SE::EnemyDestroy);

			for (int i = 0; i < 50; ++i) {
				// 短い距離でふわっと広がるように、ごく小さなランダムベクトルを作る
				RyoEngine::Vector3 particleVel = {
					(static_cast<float>(rand() % 200 - 100) / 100.0f),
					(static_cast<float>(rand() % 200 - 100) / 100.0f),
					(static_cast<float>(rand() % 200) / 100.0f)
				};
				particleVel *= 10.0f;

				particleManager_.Emit(
					model_->GetWorldPos(),  // 発生位置
					particleVel,            // その場付近でフワッと広がる速度
					1.0f,                   // 寿命（秒）
					0.4f,                  // 大きさ（スケール）
					true                   // 重力（花火のようにふわっとさせたい場合はfalse、落としたいならtrue）
				);
			}
			//isDead_ = true;
			return true;
		}

		// そうでなければfalse
		return false;
	}

	void Damage() {
		if (damageTimer_ > 0.0f) {
			damageTimer_ -= TimeManager::GetDeltaTime();
			model_->SetColor({ 1.0f,0.0f,0.0f,1.0f });
		} else {
			model_->SetColor({ 1.0f,1.0f,1.0f,1.0f });
		}
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
		// スケールを 1 から 0 へ
		model_->SetScale({ easeScale, easeScale, easeScale });

		// 2. 回転させる
		float currentRotationY = RyoEngine::EaseOutQuad(t, 0.0f, 10.0f * 2.0f * static_cast<float>(M_PI));
		model_->SetRotateY(currentRotationY); // Ｙ軸回転の場合の例
	}

	void DestroyAnimation() {
		if (!isDestroy_)return;

		animTimer_ -= TimeManager::GetDeltaTime();
		float t = animTimer_ / kDestroyDuration_;

		if (t <= 0.0f) {
			t = 0.0f;
			isDead_ = true; // 死亡
		}

		// 1. スケール用イージング（はじめ遅く終わり早く ＝ easeIn など。例えば t * t）
		float easeScale = RyoEngine::EaseInQuart(t, 0.0f, 1.0f);
		// スケールを 0 から 1 へ
		model_->SetScale({ easeScale, easeScale, easeScale });
	}

	static void ResetIdCounter() { nextEnemyId_ = 0; }

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
	float kAnimDuration_ = 1.2f;
	float kDestroyDuration_ = 0.8f;
	bool isSpawning_ = true;
	bool isDespawning_ = false;
	bool isDestroy_ = false;

	// 被弾時
	float damageTimer_ = 0.0f;
	float kDamageTimer_ = 0.15f;

	// パーティクル
	ParticleManager particleManager_;
	uint32_t particle_ = 0;
};