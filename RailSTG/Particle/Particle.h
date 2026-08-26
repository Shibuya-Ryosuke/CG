#pragma once
#include "../../Original/RyoEngine.h"

struct Particle {
    Particle() = default;
    ~Particle() = default;

    RyoEngine::Vector3 position{};
    RyoEngine::Vector3 velocity{};
    float scale = 1.0f;
    float alpha = 1.0f;
    float lifeTime = 1.0f;     // 最大寿命（秒）
    float currentLife = 0.0f;  // 現在の経過時間
    bool useGravity = false;   // 重力の有無

    float gravity = 9.8f;

    bool IsDead() const { return currentLife >= lifeTime; }
};