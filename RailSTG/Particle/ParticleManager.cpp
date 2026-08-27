#include "ParticleManager.h"
#include "../Time/TimeManager.h"
#include "../../Original/Base/DirectXCommon.h"

#include <imgui.h>
#include <algorithm>

using namespace RyoEngine;

namespace {
    // D3D12の定数バッファ(CBV)は256バイトアラインメントが必須
    constexpr uint32_t Align256(uint32_t size) {
        return (size + 255) & ~255u;
    }
}

void ParticleManager::Initialize(uint32_t handle) {
    if (!particleModel_) {
        particleModel_ = Model::Create("resources/RailSTG/Model/Particle/particle.obj");
        particleModel_->SetEnableLighting(false);
    }
    textureHandle_ = handle;

    if (!instanceWVPResource_) {
        alignedWVPStride_ = Align256(static_cast<uint32_t>(sizeof(TransformationMatrix)));

        auto device = DirectXCommon::GetInstance()->GetDevice();
        instanceWVPResource_ = DirectXCommon::CreateBufferResource(device, alignedWVPStride_ * kMaxParticles);
        instanceWVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&instanceWVPMapped_));
    }
}

void ParticleManager::Finalize() {
    if (instanceWVPResource_) {
        instanceWVPResource_->Unmap(0, nullptr);
        instanceWVPMapped_ = nullptr;
        instanceWVPResource_.Reset();
    }
    particleModel_.reset();
}

/// <summary>
/// 出現
/// </summary>
/// <param name="position">位置</param>
/// <param name="velocity">速さ</param>
/// <param name="lifeTime">寿命</param>
/// <param name="scale">大きさ</param>
/// <param name="color">色</param>
/// <param name="useGravity">重力を適用するかどうか</param>
void ParticleManager::Emit(const Vector3& position, const Vector3& velocity,
    float lifeTime, float scale, bool useGravity) {
    Particle p;
    p.position = position;
    p.velocity = velocity;
    p.lifeTime = lifeTime;
    p.currentLife = 0.0f;
    p.scale = scale;
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

        float lifeRate = p.currentLife / p.lifeTime;
        lifeRate = Clamp(lifeRate, 0.0f, 1.0f);

        p.alpha = (1.0f - lifeRate);
    }

    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(),
            [](const Particle& p) { return p.IsDead(); }),
        particles_.end()
    );

    // 専用バッファのサイズ(kMaxParticles)を超える分は今回は描画しない
    if (particles_.size() > kMaxParticles) {
        particles_.resize(kMaxParticles);
    }

    Matrix4x4 cameraRot = camera.GetViewMatrix();
    // 平行移動成分をクリアして回転だけにする
    cameraRot.m[3][0] = 0.0f;
    cameraRot.m[3][1] = 0.0f;
    cameraRot.m[3][2] = 0.0f;
    Matrix4x4 billboardRot = Transpose(cameraRot); // ビュー行列の回転の転置（逆回転）

    for (size_t i = 0; i < particles_.size(); ++i) {
        const Particle& p = particles_[i];

        // 1. スケール行列
        Matrix4x4 scaleMat = MakeScaleMatrix({ p.scale, p.scale, p.scale });
        // 2. ビルボード回転（カメラを常に向く）
        // 3. 平行移動行列
        Matrix4x4 transMat = MakeTranslateMatrix(p.position);

        // ワールド行列 ＝ スケール × カメラ向いた回転 × 位置
        Matrix4x4 world = scaleMat * billboardRot * transMat;
        Matrix4x4 wvp = world * camera.GetViewProjectionMatrix();

        auto* dst = reinterpret_cast<TransformationMatrix*>(instanceWVPMapped_ + i * alignedWVPStride_);
        dst->World = world;
        dst->WVP = wvp;
        dst->alpha = p.alpha;
    }
}

void ParticleManager::Draw() {
    if (!particleModel_ || !instanceWVPResource_) return;

#ifdef _DEBUG
    ImGui::Begin("particle");
    ImGui::Text("count: %d", static_cast<int>(particles_.size()));
#endif

    for (size_t i = 0; i < particles_.size(); ++i) {
        // i番目のパーティクル専用のCBアドレス(他のパーティクルとは独立したメモリ)を渡して描画する。
        // model_->Draw()(内部でwvpResource_を1個だけ使う版)は使わない。
        D3D12_GPU_VIRTUAL_ADDRESS wvpGVA =
            instanceWVPResource_->GetGPUVirtualAddress() + i * alignedWVPStride_;
        particleModel_->DrawInstance(wvpGVA,textureHandle_);

        const Particle& p = particles_[i];
#ifdef _DEBUG
        ImGui::Text("position: (%.2f, %.2f, %.2f)", p.position.x, p.position.y, p.position.z);
        ImGui::Text("particleColor: (%.2f)", p.alpha);
#endif
    }
#ifdef _DEBUG
    ImGui::End();
#endif
}