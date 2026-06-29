#pragma once
#include "../../Original/RyoEngine.h"

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	// 行動フェーズ
	enum class Phase {
		Approach,  // 接近する
		Leave,     // 離脱する
	};

	Enemy() = default;
	~Enemy();

	void Initialize(const Vector3& position, const Vector3& velocity);
	void Update(RyoEngine::Camera& camera);
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

	float GetMoveSpeed() { return kMoveSpeed_; }

private:
	void PhaseApproach();
	void PhaseLeave();
	void UpdatePhase();

private:
	// 自身
	RyoEngine::Model* model_ = nullptr;
	// フェーズ
	Phase phase_ = Phase::Approach;

	// 移動速度
	static constexpr float kMoveSpeed_ = 0.1f;
	// 接近フェーズ時の速度倍率
	static constexpr float kApproachSpeedRate_ = 1.0f;
	// 離脱フェーズ時の速度倍率
	static constexpr float kLeaveSpeedRate_ = 1.8f;

	// 速度
	Vector3 velocity_{};
};