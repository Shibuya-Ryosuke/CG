#pragma once
#include <memory>
#include <vector>
#include "../BaseEnemy.h"
#include "../EnemyEnum.h"

class EnemyBullet;

class HomingMob : public BaseEnemy {
public:
    HomingMob();
    ~HomingMob();
    HomingMob(const HomingMob&) = delete;
    HomingMob& operator=(const HomingMob&) = delete;

    void Initialize() override;
    void Finalize() override;
    void Update(const RyoEngine::Camera& camera) override;
    void Draw() override;

    void Shot();
    void Move();

    void UpdateDeflectedBullets(const std::function<BaseEnemy* (int32_t)>& enemyFinder);

    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const override { return bullets_; }
    void SetTargetPos(const RyoEngine::Vector3& targetPos) { targetPos_ = targetPos; }

private:
    float kShotInterval = 3.0f;
    float kBulletSpeed = 20.0f;      // Mobより低速
    float kBulletDamage = 25.0f;     // Mobより高威力
    RyoEngine::Vector3 kVelocity{ 0.0f,0.0f,6.0f };
    float kMaxHp_ = 100.0f;
    float kHomingBulletHp = 20.0f;
private:
    std::vector<std::unique_ptr<EnemyBullet>> bullets_;
    float shotInterval_ = kShotInterval;
    RyoEngine::Vector3 targetPos_{};
    MobState state_ = MobState::None;
    MobState request_ = MobState::None;
};