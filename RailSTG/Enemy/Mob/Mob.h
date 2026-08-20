#pragma once
#include <memory>
#include <functional>

#include "../BaseEnemy.h"
#include "../EnemyEnum.h"

class EnemyBullet;

class Mob : public BaseEnemy {
public:
	Mob();
	~Mob();
	// コピーコンストラクタと代入演算子の明示的削除
	Mob(const Mob&) = delete;
	Mob& operator=(const Mob&) = delete;

	/// <summary>
	/// 更新
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="camera"></param>
	void Update(const RyoEngine::Camera& camera) override;

	/// <summary>
	/// 描画
	/// </summary>
	void Draw() override;

	/// <summary>
	/// 射撃
	/// </summary>
	void Shot();

	void Move();

	void UpdateDeflectedBullets(const std::function<BaseEnemy* (int32_t)>& enemyFinder);
	
	const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const override {
		return bullets_;
	}

	void SetTargetPos(const RyoEngine::Vector3& targetPos) { targetPos_ = targetPos; }

private:
	float kShotInterval = 2.5f;
	float kBulletSpeed = 36.0f;
	RyoEngine::Vector3 kVelocity{ 0.0f,0.0f,0.0f };

	float kMaxHp_ = 50.0f;

	float kBulletDamage = 10.0f;
	float kBulletTurnRate = 0.36f;

private:
	// 弾
	std::vector<std::unique_ptr<EnemyBullet>> bullets_;
	// 射撃間隔
	float shotInterval_ = kShotInterval;

	// ターゲット（プレイヤー）の座標
	RyoEngine::Vector3 targetPos_{};

	// 状態
	MobState state_ = MobState::None;
	MobState request_ = MobState::None;
};