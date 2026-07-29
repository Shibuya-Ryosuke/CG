#pragma once
#include "../../Original/RyoEngine.h"
#include "../EnemyBullet/EnemyBullet.h"

class IEnemyState;
class Player;

/// <summary>
/// 敵
/// </summary>
class Enemy {
public:
	
	Enemy() = default;
	~Enemy();

	void Initialize(const Vector3& position, const Vector3& velocity);
	void Update(RyoEngine::Camera& camera);
	void Update(RyoEngine::DebugCamera& debugCamera);
	void Draw();

	void ChangeState(IEnemyState* newState);

	void MoveTranslate(const Vector3& translation); // 移動させる
	
	// 接近フェーズ初期化
	void ApproachPhaseInitialize() { fireTimer = kFireInterval_; }

	void CountDownFire();

	/// <summary>
	/// 弾発射
	/// </summary>
	void Fire();
	
	Vector3 GetWorldposition() {
		Vector3 worldPos{
			.x = model_->GetWorldMatrix().m[3][0],
			.y = model_->GetWorldMatrix().m[3][1],
			.z = model_->GetWorldMatrix().m[3][2],
		};
		return worldPos;
	}

	float GetPositionZ() const;                     // Z座標を取得する
	Vector3 GetVelocity() const { return velocity_; }
	float GetApproachSpeedRate() const { return kApproachSpeedRate_; }
	float GetLeaveSpeedRate() const { return kLeaveSpeedRate_; }
	float GetMoveSpeed() { return kMoveSpeed_; }

	void SetPlayer(Player* player) { player_ = player; }

private:
	void UpdateState();

private:
	// プレイヤー
	Player* player_ = nullptr;

	// 自身
	RyoEngine::Model* model_ = nullptr;
	// ステート
	IEnemyState* state_ = nullptr;

	// 移動速度
	static constexpr float kMoveSpeed_ = 0.1f;
	// 接近フェーズ時の速度倍率
	static constexpr float kApproachSpeedRate_ = 1.0f;
	// 離脱フェーズ時の速度倍率
	static constexpr float kLeaveSpeedRate_ = 1.8f;

	// 速度
	Vector3 velocity_{};

	// 弾
	std::list<EnemyBullet*> bullets_;
	// 発射間隔
	static const int32_t kFireInterval_ = 60;
	// 発射タイマー
	int32_t fireTimer = 0;
};