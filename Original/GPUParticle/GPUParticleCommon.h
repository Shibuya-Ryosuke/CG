#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <vector>
#include <functional>
#include "../Math/Math.h"

namespace RyoEngine {

    // GPU上で1粒ぶんを表すデータ。コンピュート/頂点シェーダー側のParticleData構造体と
    // フィールドの並び順・型を1:1で合わせること。ライティングに関する情報(法線等)は一切持たない。
    // NOTE: float3の直後のfloatを、16byte境界に収まるよう別の値で埋めている(パディングを無駄にしない並び)
    struct GPUParticleData {
        Vector3 position;
        float   scale;
        Vector3 velocity;
        float   totalLife;      // 発生時の寿命(秒)。フェードの割合計算に使う
        Vector4 color;
        Vector3 rotation;       // オイラー角(ラジアン)。ビルボード時はz(カメラ正面軸まわりのロール)のみ使う
        float   remainingLife;  // 残り寿命(秒)。0以下になったら「死んでいる」扱い
    };
    static_assert(sizeof(GPUParticleData) == 64, "GPUParticleData構造体のサイズがシェーダー側と食い違うと壊れるので確認すること");

    // コンピュートシェーダーへ渡す、毎フレーム変わるパラメータ(CBV)
    struct GPUParticleSimParams {
        float    deltaTime;
        uint32_t spawnRequestCount; // 今フレームの発生依頼数
        uint32_t maxParticleCount;  // このEmitterのパーティクル総数(スロット数)
        float    gravity;           // 下向き(-Y)の加速度。0なら無重力
    };

    // 描画時に頂点シェーダーへ渡すカメラ情報(CBV)
    struct GPUParticleCameraData {
        Matrix4x4 viewProjection;
        Vector3   cameraRight;  // ビルボード計算用(ワールド空間でのカメラの右方向)
        float     padding0;
        Vector3   cameraUp;     // ビルボード計算用(ワールド空間でのカメラの上方向)
        float     padding1;
    };

    /// <summary>
    /// GPUパーティクル(3D)の
    /// ・シミュレーション用コンピュートシェーダー(リセットパス/本体パス)
    /// ・描画用シェーダー(ビルボードON/OFF × 加算/通常合成の4パターン)
    /// のRootSignature/PSOをまとめて管理するクラス。
    ///
    /// 各GPUParticleEmitterが持つバッファ(パーティクル本体/発生依頼/カウンター)は
    /// すべてRoot Descriptor(SRV/UAV/CBVを直接バインド)でやり取りするため、
    /// このクラスも各Emitterも、専用のディスクリプタヒープを一切必要としない。
    /// (Textureのみ、既存のTextureManagerのヒープを使ったDescriptorTableで渡す)
    ///
    /// ModelCommon/InstancedModelCommonと同様、実際のGPUコマンドはその場では発行せず、
    /// SetDispatchCommands()/SetDrawCommands()で予約しておき、EndFrame()内の正しいタイミングで
    /// Dispatch()/Draw()がまとめて実行する。
    /// </summary>
    class GPUParticleCommon {
    public:
        enum class BlendMode {
            Alpha,    // 通常の半透明合成(煙など)
            Additive, // 加算合成(炎・火花など)
        };

        static GPUParticleCommon* GetInstance();

        void Initialize();
        void Finalize();

        // --- コンピュート ---
        ID3D12RootSignature* GetComputeRootSignature() const { return computeRootSignature_.Get(); }
        ID3D12PipelineState* GetResetPipelineState() const { return resetPipelineState_.Get(); }
        ID3D12PipelineState* GetSimulatePipelineState() const { return simulatePipelineState_.Get(); }

        // --- 描画 ---
        ID3D12RootSignature* GetRenderRootSignature() const { return renderRootSignature_.Get(); }
        ID3D12PipelineState* GetRenderPipelineState(bool billboard, BlendMode blendMode) const {
            int index = (billboard ? 1 : 0) + (blendMode == BlendMode::Additive ? 2 : 0);
            return renderPipelineStates_[index].Get();
        }

        // --- 予約キュー ---
        // シミュレーション(Dispatch)の予約。EndFrame()の早い段階(シャドウパスの前あたり)で実行すること。
        void SetDispatchCommands(const std::function<void()>& function) { dispatchCommands_.push_back(function); }
        // 描画の予約。3Dシーンパス中(ModelCommon::Draw()等と同じタイミング)で実行すること。
        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }

        void Dispatch();
        void Draw();

        // 予約をクリアする(NewFrame()の先頭で呼ぶこと)
        void CommandsClear() { dispatchCommands_.clear(); drawCommands_.clear(); }

    private:
        GPUParticleCommon() = default;
        ~GPUParticleCommon() = default;
        GPUParticleCommon(const GPUParticleCommon&) = delete;
        GPUParticleCommon& operator=(const GPUParticleCommon&) = delete;

        void CreateComputeRootSignature();
        void CreateComputePipelineStates();
        void CreateRenderRootSignature();
        void CreateRenderPipelineStates();

        // --- コンピュート用 ---
        Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> resetPipelineState_;    // カウンターを0に戻すだけの1スレッド用パス
        Microsoft::WRL::ComPtr<ID3D12PipelineState> simulatePipelineState_; // 物理演算＋新規発生の消化を行う本体パス

        // --- 描画用 ---
        Microsoft::WRL::ComPtr<ID3D12RootSignature> renderRootSignature_;
        // インデックス: 0=通常合成/非ビルボード, 1=通常合成/ビルボード, 2=加算合成/非ビルボード, 3=加算合成/ビルボード
        Microsoft::WRL::ComPtr<ID3D12PipelineState> renderPipelineStates_[4];

        // --- 予約キュー ---
        std::vector<std::function<void()>> dispatchCommands_;
        std::vector<std::function<void()>> drawCommands_;
    };
}
