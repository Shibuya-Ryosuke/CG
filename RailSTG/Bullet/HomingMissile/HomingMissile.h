#pragma once
#include "../../../Original/RyoEngine.h"
#include "../BaseBullet.h"
#include <cstdint>

// 前方宣言（あるいはインクルード）
class BaseEnemy;

class HomingMissile : public BaseBullet {
public:
    HomingMissile();
    ~HomingMissile() override = default;

    HomingMissile(const HomingMissile&) = delete;
    HomingMissile& operator=(const HomingMissile&) = delete;

    // ターゲットを指定して初期化できるようにする
    void Initialize() override; // オーバーライド元の都合に合わせて調整
    void Initialize(const RyoEngine::Vector3& spawnPos, BaseEnemy* targetEnemy);
    void Finalize() override;

    void Update(const RyoEngine::Camera& camera) override;
    void Draw() override;

private:
    BaseEnemy* targetEnemy_ = nullptr; // 追尾する敵のポインタ
    float turnRate_ = 1.0f;           // 旋回性能（大回り具合）
    float speed_ = 60.0f;              // ミサイルの移動速度
};