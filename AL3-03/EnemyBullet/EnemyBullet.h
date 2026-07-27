#pragma once
#pragma once
#include "../../Original/RyoEngine.h"

class EnemyBullet {
public:
	EnemyBullet() = default;
	~EnemyBullet();

	void Initialize(const Vector3& position, const Vector3& velocity);

	void Update(const RyoEngine::Camera& camera);
	void Update(const RyoEngine::DebugCamera& debugCamera);

	void Draw();

	float GetBulletSpeed() { return kBulletSpeed_; };
	bool IsDead() const { return isDead_; };

	void SetTranslate(const Vector3& translate) { model_->SetTranslate(translate); };
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

private:
	// 弾の速度
	static constexpr float kBulletSpeed_ = 1.0f;
	// 寿命
	static const int32_t kLifeTime_ = 60 * 5;

	// 自身
	RyoEngine::Model* model_ = nullptr;

	// 速度
	Vector3 velocity_{};

	// デスタイマー
	int32_t deathTimer_ = kLifeTime_;
	// デスフラグ
	bool isDead_ = false;
};
