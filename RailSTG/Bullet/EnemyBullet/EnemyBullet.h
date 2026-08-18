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
	void SetIsDeflected(bool isDefrected) { isDeflected_ = isDefrected; }

	bool IsDeflectable() { return isDeflectable_; }
	bool IsDeflected() { return isDeflected_; }

	void SetOwnerId(int32_t id) { ownerId_ = id; }
	int32_t GetOwnerId() const { return ownerId_; }

	void SetTargetPosition(const RyoEngine::Vector3& pos) { targetPosition_ = pos; hasTarget_ = true; }
	void ClearTarget() { hasTarget_ = false; }

	void CaptureSpeedForDeflection() { baseSpeed_ = RyoEngine::Length(velocity_); }

private:
	// プレイヤーが反射可能か
	bool isDeflectable_ = false;
	// 跳ね返されたか
	bool isDeflected_ = false;
	// 跳ね返されたときのスピードは1.5倍
	float deflectedSpeedScale_ = 1.5f;

	// 発射元ID
	int32_t ownerId_ = -1;

	// 追尾先座標
	RyoEngine::Vector3 targetPosition_{};
	bool hasTarget_ = false;

	float baseSpeed_ = 0.0f;
};