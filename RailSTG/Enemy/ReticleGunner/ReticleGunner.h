#pragma once
#include <array>
#include <vector>
#include <memory>
#include "../BaseEnemy.h"
#include "../EnemyEnum.h"
#include "../../Bullet/EnemyBullet/EnemyBullet.h"

struct ReticleData {
    ReticleState state = ReticleState::None;
    RyoEngine::Vector2 screenPos{};
    RyoEngine::Vector3 worldPos{};
    float stateTimer = 0.0f;
};

class ReticleGunner : public BaseEnemy {
public:
    ReticleGunner();
    ~ReticleGunner();
    ReticleGunner(const ReticleGunner&) = delete;
    ReticleGunner& operator=(const ReticleGunner&) = delete;

    void Initialize() override;
    void Finalize() override;
    void Update(const RyoEngine::Camera& camera) override;
    void Draw() override;

    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const override {
        static const std::vector<std::unique_ptr<EnemyBullet>> kEmpty;
        return kEmpty;
    }

    // Game側から毎フレームプレイヤーのワールド座標をもらう(Mob::SetTargetPosと同じパターン)
    void SetPlayerWorldPos(const RyoEngine::Vector3& pos) { playerWorldPos_ = pos; }

    bool TryJudgeHit(const RyoEngine::Vector2& playerScreenPos);

    float GetDamage() const { return kDamage_; }

private:
    void UpdateReticles(const RyoEngine::Camera& camera);
    void UpdateAllLockedPhase();

private:
    float kNoneDuration_ = 1.0f;
    float kFollowDuration_ = 1.0f; // 仮値：追従してから固定するまでの秒数
    float kLockedWaitDuration_ = 1.0f;  // 3個目Locked後、Readyになるまでの待機時間(仮)
    float kReadyDuration_ = 0.5f; // 仮値：予告SEからダメージ判定までの秒数
    float kShotDuration_ = 0.3f;
    float kEndDuration_ = 4.0f;
    float kMaxHp_ = 150.0f;
    float kHitRadius_ = 25.0f; // 仮値：レティクルの当たり判定半径(スクリーン座標系のピクセル数)
    float kDamage_ = 40.0f;

private:
    std::array<ReticleData, 3> reticles_{};
    int activeIndex_ = 0; // 現在Following/None処理中のレティクル番号(0〜2)。3になったら全部Locked済み

    // Locked→Ready→Shotの一括進行用タイマー
    float allLockedTimer_ = 0.0f;

    bool judged_ = false; // Shot状態での判定を実行済みかどうか

    RyoEngine::Vector3 playerWorldPos_{};
};