#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include "GPUParticleCommon.h"
#include "../3D/Mesh/InstancedMesh.h"

namespace RyoEngine {
    class Camera;

    /// <summary>
    /// GPUパーティクル(3D)の「1つの独立したエフェクトの群れ」。
    /// 炎用・煙用・爆発用など、用途ごとに複数個作って個別に管理することを想定している。
    ///
    /// 位置・速度・寿命などのシミュレーションはCPUではなくコンピュートシェーダーが行う。
    /// CPU側は「発生依頼を積む」「deltaTimeを渡す」だけで、どのスロットが空いているかを
    /// 把握する必要はない(死んでいるスロットが自分でGPU上で発生依頼を拾いにいく方式のため)。
    ///
    /// NOTE: 現状は「上限数ぶん常に存在させておき、毎フレーム全スロットをDispatch/Drawする」
    ///       簡易版。死んでいるスロットは寿命フェード(remainingLife/totalLife)により
    ///       自動的にアルファ0(完全に透明)として描画されるため、見た目には出てこない。
    /// </summary>
    class GPUParticleEmitter {
    public:
        /// <summary>
        /// 初期化。
        /// </summary>
        /// <param name="meshFilePath">1粒の形状となる単一メッシュのOBJファイルパス(板ポリゴン等)</param>
        /// <param name="maxParticleCount">このEmitterが同時に持てる最大数(GPUバッファの固定サイズ)</param>
        /// <param name="billboard">true:常にカメラを向く / false:rotationで自分の姿勢のまま回転する</param>
        /// <param name="blendMode">Alpha(通常合成)かAdditive(加算合成)か</param>
        void Initialize(const std::string& meshFilePath, uint32_t maxParticleCount,
            bool billboard = true, GPUParticleCommon::BlendMode blendMode = GPUParticleCommon::BlendMode::Additive);
        void Finalize();

        /// <summary>
        /// パーティクルを1個発生させるよう依頼する。実際にGPU上へ反映されるのはUpdate()内。
        /// 1フレームあたりの依頼数には上限(kMaxSpawnRequestsPerFrame)がある。
        /// </summary>
        void Emit(const Vector3& position, const Vector3& velocity, const Vector4& color,
            float scale, const Vector3& rotation, float lifeTime);

        // 重力(下向き、-Y方向の加速度)。0なら無重力。Emitter全体で共通の設定。
        void SetGravity(float gravity) { gravity_ = gravity; }
        float GetGravity() const { return gravity_; }

        /// <summary>
        /// シミュレーションの実行を予約する。deltaTimeを渡すこと。
        /// NOTE: Model::Draw()等と同じく、呼んだ直後にGPUコマンドが発行されるわけではない。
        ///       実際のDispatchは、EndFrame()内でGPUParticleCommon::Dispatch()がまとめて実行する
        ///       タイミング(シャドウパスより前)で行われる。
        /// </summary>
        void Update(float deltaTime);

        /// <summary>
        /// 描画を予約する。
        /// NOTE: 実際の描画は、EndFrame()内でGPUParticleCommon::Draw()がまとめて実行する
        ///       タイミング(3Dシーンパス中)で行われる。カメラの情報だけはこの時点で確定させておく。
        /// </summary>
        void Draw(const Camera& camera);

    private:
        void InternalDispatch();
        void InternalDraw();
        void TransitionParticleBuffer(D3D12_RESOURCE_STATES newState);

        std::shared_ptr<InstancedMesh> mesh_;
        uint32_t maxParticleCount_ = 0;
        bool billboard_ = true;
        GPUParticleCommon::BlendMode blendMode_ = GPUParticleCommon::BlendMode::Additive;
        float gravity_ = 0.0f;

        // パーティクル本体 (DEFAULT heap、UAV/SRV両対応のStructuredBuffer。maxParticleCount_件ぶん固定確保)
        Microsoft::WRL::ComPtr<ID3D12Resource> particleResource_;
        D3D12_RESOURCE_STATES particleState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        // 発生依頼バッファ (Upload heap、CPUがマップして毎フレーム書き込む)
        static constexpr uint32_t kMaxSpawnRequestsPerFrame = 256;
        Microsoft::WRL::ComPtr<ID3D12Resource> spawnRequestResource_;
        GPUParticleData* spawnRequestData_ = nullptr;
        std::vector<GPUParticleData> pendingSpawnRequests_; // Emit()が積む、CPU側の一時リスト

        // 発生依頼消化用のAtomicカウンター (DEFAULT heap、UAV。中身はuint1個)
        Microsoft::WRL::ComPtr<ID3D12Resource> claimCounterResource_;

        // シミュレーションパラメータ用バッファ (Upload heap)
        Microsoft::WRL::ComPtr<ID3D12Resource> simParamsResource_;
        GPUParticleSimParams* simParamsData_ = nullptr;

        // カメラ用バッファ (Upload heap)
        Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
        GPUParticleCameraData* cameraData_ = nullptr;
    };
}
