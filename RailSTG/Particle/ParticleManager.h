#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>
#include "../../../Original/RyoEngine.h"
#include "Particle.h"

class ParticleManager {
public:
    ParticleManager() = default;
    ~ParticleManager() = default;

    void Initialize(uint32_t handle); // 引数追加。ここでModel::Create(filePath)する
    void Finalize();
    void Update(const RyoEngine::Camera& camera);
    void Draw();
    void Emit(const RyoEngine::Vector3& position, const RyoEngine::Vector3& velocity,
        float lifeTime, float scale, const RyoEngine::Vector4& color, bool useGravity = false);

private:

    // 同時に描画できるパーティクルの最大数(専用WVPバッファのサイズを決める)
    // 1000個は安定して出したいとのことなので、多少の余裕を見て1536にしてある
    static constexpr uint32_t kMaxParticles = 1536;

    std::vector<Particle> particles_;
    inline static std::unique_ptr<RyoEngine::Model> particleModel_ = nullptr;

    //RyoEngine::Camera camera_;

    // パーティクル1体につき1個分のTransformationMatrix(World/WVP)を書き込む専用バッファ。
    // Modelクラスは自身のwvpResource_を1個しか持たないため、1体のModelを使い回すと
    // 全パーティクルが「最後に書き込まれた1つの行列」を共有してしまう(＝1個しか描画されない不具合の原因)。
    // ここではパーティクルごとに独立したメモリ領域(=CBV)を用意することで、
    // 各DrawInstance呼び出しが互いに独立した行列を参照できるようにする。
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceWVPResource_;
    uint8_t* instanceWVPMapped_ = nullptr;
    uint32_t alignedWVPStride_ = 0;
};