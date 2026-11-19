#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <memory>
#include <vector>
#include "../../Core/Math/Math.h"

namespace RyoEngine {

    /// <summary>
    /// コンピュートシェーダーでスキニングするクラス。
    /// アニメーションするモデル1体ごとに「スキニング後の頂点バッファ」を持ち、
    /// 毎フレーム、姿勢が変わったモデルだけスキニングし直す。
    /// できた頂点バッファは普通のモデルと同じパイプラインで描くので、影・カスタムシェーダーもそのまま使える。
    ///
    /// ・CreateInstance / ReleaseInstance : Model が作る・消す (バッファはGPUが使い終わるまで数フレーム残す)
    /// ・Request : Model::Draw から、行列パレットを書いてスキニングを予約する
    /// ・Dispatch: RyoEngine::EndFrame で、影・3D描画の前にまとめて実行する
    /// </summary>
    class SkinningManager {
    public:
        static constexpr uint32_t kInvalidInstance = 0;

        static void Initialize();
        static void Finalize();
        /// <summary>使い終わったバッファを、時間が経ったものから解放する (NewFrame で呼ばれる)</summary>
        static void NewFrame();

        /// <summary>モデル1体分のバッファを作る (骨格を持つモデルの番号)。失敗時は kInvalidInstance</summary>
        static uint32_t CreateInstance(uint32_t modelIndex);
        /// <summary>モデル1体分のバッファを返す (GPUが使い終わるまで数フレーム残してから解放する)</summary>
        static void ReleaseInstance(uint32_t instance);

        /// <summary>
        /// メッシュ1つ分のスキニングを予約する。jointMatrices は各ボーンの今の行列 (Animator::GetJointMatrices)
        /// </summary>
        static void Request(uint32_t instance, uint32_t meshIndex, const std::vector<Matrix4x4>& jointMatrices);

        /// <summary>
        /// スキニング後の頂点バッファ。まだ一度もスキニングしていなければ nullptr
        /// (そのときはモデルデータの、元の姿勢の頂点で描く)
        /// </summary>
        static const D3D12_VERTEX_BUFFER_VIEW* GetVertexBufferView(uint32_t instance, uint32_t meshIndex);

        /// <summary>予約されたスキニングをまとめて実行する (EndFrame内、影・3D描画の前)</summary>
        static void Dispatch();

    private:
        static SkinningManager* GetInstance();

        SkinningManager() = default;
        ~SkinningManager() = default;
        SkinningManager(const SkinningManager&) = delete;
        SkinningManager& operator=(const SkinningManager&) = delete;

        void CreateRootSignature();
        void CreatePipelineState();

        // 行列パレットの1要素 (Skinning.CS.hlsl の Well と合わせる)
        struct Well {
            Matrix4x4 skeletonSpaceMatrix;                 // 頂点を動かす行列
            Matrix4x4 skeletonSpaceInverseTransposeMatrix; // 法線を動かす行列 (逆転置)
        };

        // メッシュ1つ分のバッファ
        struct MeshBuffers {
            Microsoft::WRL::ComPtr<ID3D12Resource> input;   // スキニングの入力 (モデルデータのもの。共有)
            Microsoft::WRL::ComPtr<ID3D12Resource> output;  // スキニング後の頂点 (UAV → 頂点バッファ)
            Microsoft::WRL::ComPtr<ID3D12Resource> palette; // 行列パレット (毎フレーム CPU から書く)
            Well* paletteMapped = nullptr;
            uint32_t paletteCount = 0;
            uint32_t vertexCount = 0;
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            bool computed = false;        // 一度でもスキニングを予約したか
            uint64_t requestedFrame = 0;  // 最後に予約されたフレーム (同じフレームで2回予約しないため)
        };
        // モデル1体分
        struct Instance {
            uint32_t modelIndex = 0;
            std::vector<MeshBuffers> meshes; // ModelAsset::meshes と同じ順番 (スキニングしないメッシュは空のまま)
        };
        // 使い終わったバッファ (GPUが使い終わるまで数フレーム残す)
        struct RetiredInstance {
            std::unique_ptr<Instance> instance;
            int framesLeft = 0;
        };
        struct RequestEntry {
            uint32_t instance;
            uint32_t meshIndex;
        };

        Instance* Find(uint32_t instance);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

        std::vector<std::unique_ptr<Instance>> instances_; // instance-1 が添字。空いた所は nullptr
        std::vector<uint32_t> freeInstances_;
        std::vector<RetiredInstance> retired_;
        std::vector<RequestEntry> requests_;

        // 1グループのスレッド数 (Skinning.CS.hlsl の numthreads と合わせる)
        static constexpr uint32_t kThreadGroupSize = 256;
        // 使い終わったバッファを残すフレーム数
        static constexpr int kRetireFrames = 3;
    };
}
