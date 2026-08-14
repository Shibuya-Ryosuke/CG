#pragma once
#include <memory>

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


	const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const {
		return bullets_;
	}

private:
	float kShotInterval = 1.0f;
	float kBulletSpeed = 120.0f;
	RyoEngine::Vector3 kVelocity{ 0.0f,0.0f,6.0f };

private:
	// 弾
	std::vector<std::unique_ptr<EnemyBullet>> bullets_;
	// 射撃間隔
	float shotInterval_ = kShotInterval;

	// 状態
	MobState state_ = MobState::None;
	MobState request_ = MobState::None;
};