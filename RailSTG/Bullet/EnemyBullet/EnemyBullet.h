#pragma once
#include "../BaseBullet.h"
#include <cstdint>

class EnemyBullet : public BaseBullet {
public:
	EnemyBullet();
	~EnemyBullet() override = default;
	// コピーコンストラクタと代入演算子の明示的削除
	EnemyBullet(const EnemyBullet&) = delete;
	EnemyBullet& operator=(const EnemyBullet&) = delete;

	void Initialize() override;
	void Finalize() override;

	void Update(const RyoEngine::Camera& camera) override;
	void Draw() override;

	void SetIsDeflectable(bool isDefrectable) { isDeflectable_ = isDefrectable; }
	void SetIsDeflected(bool isDefrected) {
		isDeflected_ = isDefrected;
		if (isDefrected) {
			isHomingDisabled_ = false;
		}
	}

	bool IsDeflectable() const { return isDeflectable_; }
	bool IsDeflected() const { return isDeflected_; }

	void SetOwnerId(int32_t id) { ownerId_ = id; }
	int32_t GetOwnerId() const { return ownerId_; }

	void SetTargetPos(const RyoEngine::Vector3& pos) {
		if (isHomingDisabled_) return; // 追尾終了後は外部からの再セットを無視
		targetPos_ = pos;
		hasTarget_ = true;
	}
	void ClearTarget() { hasTarget_ = false; }

	void SetIsDestructible(bool isDestructible) { isDestructible_ = isDestructible; }
	bool IsDestructible() const { return isDestructible_; }

	void OnCollisionDestructibleBullet(float damage = 0.0f) { hp_ -= damage; }

	void SetTurnRate(float turnRate) { turnRate_ = turnRate; }

private:
	// プレイヤーが反射可能か
	bool isDeflectable_ = false;
	// 跳ね返されたか
	bool isDeflected_ = false;

	// 発射元ID
	int32_t ownerId_ = -1;

	// 追尾先座標
	RyoEngine::Vector3 targetPos_{};
	bool hasTarget_ = false;

	// 撃ち落とせるかどうか
	bool isDestructible_ = false;

	// 跳ね返されたときの速度
	float kDeflectedSpeed = 180.0f;

	// 通常弾の追尾の強さ
	float turnRate_ = 0.1f;

	// ターゲットを通り過ぎたと判定する距離
	float kTargetDistance = 5.0f;

	bool isHomingDisabled_ = false;
};