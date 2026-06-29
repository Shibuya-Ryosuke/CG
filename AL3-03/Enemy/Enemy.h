#pragma once
#include "../../Original/RyoEngine.h"

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	Enemy() = default;
	~Enemy() = default;

	void Initialize(const Vector3& position, const Vector3& velocity);
	void Update(RyoEngine::Camera& camera);
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

	float GetMoveSpeed() { return kMoveSpeed_; }

private:
	// 自身
	RyoEngine::Model* model_ = nullptr;

	// 移動速度
	static constexpr float kMoveSpeed_ = 0.1f;

	// 速度
	Vector3 velocity_{};
};