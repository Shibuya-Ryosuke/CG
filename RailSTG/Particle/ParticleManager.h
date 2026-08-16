#pragma once
#include <vector>
#include <memory>
#include "../../../Original/RyoEngine.h"
#include "Particle.h"

class ParticleManager {
public:
    static ParticleManager& GetInstance() {
        static ParticleManager instance;
        return instance;
    }

    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;

    void Initialize();
    void Finalize();
    void Update(const RyoEngine::Camera& camera);
    void Draw();
    void Emit(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity,
        float lifeTime, float scale, const RyoEngine::Vector4& color, bool useGravity = false);

private:
    ParticleManager() = default;
    ~ParticleManager() = default;

    std::vector<Particle> particles_;
    std::unique_ptr<RyoEngine::Model> model_ = nullptr;

    RyoEngine::Camera camera_;
};