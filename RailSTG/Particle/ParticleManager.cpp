#include "ParticleManager.h"
#include "../Time/TimeManager.h"

#include <imgui.h>

using namespace RyoEngine;

void ParticleManager::Initialize() {
    if (!model_) {
        model_ = Model::Create("resources/RailSTG/Particle/particle.obj");
        model_->SetTex("resources/RailSTG/Particle/particle.png");
    }
}

void ParticleManager::Finalize() {
    model_.reset();
}

void ParticleManager::Emit(const Vector3& position, const Vector3& velocity,
    float lifeTime, float scale, const Vector4& color, bool useGravity) {
    Particle p;
    p.position = position;
    p.velocity = velocity;
    p.lifeTime = lifeTime;
    p.currentLife = 0.0f;
    p.scale = scale;
    p.color = color;
    p.useGravity = useGravity;
    particles_.push_back(p);
}

void ParticleManager::Update(const RyoEngine::Camera& camera) {
    for (auto& p : particles_) {
        p.currentLife += TimeManager::GetDeltaTime();

        if (p.useGravity) {
            p.velocity.y -= p.gravity * TimeManager::GetDeltaTime();
        }
        p.position += p.velocity * TimeManager::GetDeltaTime();
    }

    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [](const Particle& p) { return p.IsDead(); }),
        particles_.end()
    );

    camera_ = camera;
}

void ParticleManager::Draw() {
    if (!model_) return;

    ImGui::Begin("particle");

    for (const auto& p : particles_) {
        // 1. パーティクルごとの行列をモデルに適用
        model_->SetTranslate(p.position);
        model_->SetScale({ p.scale, p.scale, p.scale });

        // 2. モデルの内部行列を更新 (WVPを再計算)
        model_->Update(camera_);

        // 3. 描画命令を積む
        model_->Draw();

        ImGui::Text("position: (%.2f, %.2f, %.2f)", p.position.x, p.position.y, p.position.z);
    }
    ImGui::End();
}