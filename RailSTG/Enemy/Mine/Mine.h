#pragma once
#pragma once
#include <memory>
#include "../BaseEnemy.h"
#include "../../Bullet/EnemyBullet/EnemyBullet.h"

class Mine : public BaseEnemy {
public:
    Mine();
    ~Mine();
    Mine(const Mine&) = delete;
    Mine& operator=(const Mine&) = delete;

    void Initialize() override;
    void Finalize() override;
    void Update(const RyoEngine::Camera& camera) override;
    void Draw() override;

    // 機雷は弾を持たないので空を返す
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const override {
        static const std::vector<std::unique_ptr<EnemyBullet>> kEmpty;
        return kEmpty;
    }

    float GetDamage() const { return kDamage_; }

    /// <summary>
    /// プレイヤーに当たった瞬間即死亡
    /// </summary>
    void OnPlayerCollision() { isDead_ = true; }

private:
    float kApproachSpeed_ = 8.0f;   // カメラに近づく速度
    float kDespawnZ_ = -5.0f;      // これを下回ったら消去(カメラを通り過ぎた)
    float kMaxHp_ = 20.0f;
    float kDamage_ = 50.0f;

    RyoEngine::Vector3 rotation_{};
};