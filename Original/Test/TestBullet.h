#pragma once
#include "../RyoEngine.h"

namespace Test {
    class TestBullet {
    public:
        void Initialize(RyoEngine::InstancedModel* owner, const RyoEngine::Vector3& position);
        void Update();
        void ResolveDead();

        // 削除判定用
        bool IsDead() const { return isDead_; }
    private:
        RyoEngine::InstancedModel* owner_ = nullptr;
        RyoEngine::InstancedModel::Handle handle_ = RyoEngine::InstancedModel::kInvalidHandle;
        RyoEngine::Transform transform_{};
        RyoEngine::Vector3 velocity_{};
        bool isDead_ = false;

        float timer_ = 10.0f;
    };
}