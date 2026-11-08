#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "../../Core/Math/Math.h"

namespace RyoEngine {

    /// <summary>
    /// アプリのシェーダーで作るポストエフェクト (カスタムポストエフェクト) を管理・描画するクラス。
    ///
    /// ・かかる場所: トーンマッピングの後 (PostProcess::Composite の後)、UI(2D)を描く前。UIにはかからない
    /// ・複数の効果は Add した順に重ねてかかる (前の効果の結果を、次の効果が SampleScene で読む)
    /// ・作業用テクスチャ2枚を交互に読み書きし、最後の効果だけがゲーム画面へ直接書く
    /// ・有効な効果が1つも無いときは何もしない (Composite が今まで通りゲーム画面へ直接書く)
    /// ・シーンが切り替わると、登録した効果は全部消える (SceneManager が Clear() を呼ぶ)
    ///
    /// 使い方:
    ///   uint32_t shader = ShaderManager::LoadPostEffect("Vignette");
    ///   uint32_t vignette = PostEffectManager::Add(shader);
    ///   PostEffectManager::SetParam(vignette, 0, 0.6f);
    ///   PostEffectManager::SetEnabled(vignette, false);  // 一時的にオフ
    /// </summary>
    class PostEffectManager {
    public:
        static constexpr uint32_t kInvalidEffect = 0;

        static void Initialize();
        static void Finalize();

        // ---------------- 登録 ----------------
        /// <summary>
        /// 効果を登録する (最後に追加される)。ShaderManager::LoadPostEffect の戻り値を渡す。
        /// 同じシェーダーを複数登録してもよい (自由な値を変えて2回かける、など)
        /// </summary>
        /// <returns>効果の番号 (1以上)。SetParam などで使う</returns>
        static uint32_t Add(uint32_t shader);
        /// <summary>効果を削除する</summary>
        static void Remove(uint32_t effect);
        /// <summary>全部削除する (シーン切り替え時に SceneManager から呼ばれる)</summary>
        static void Clear();

        // ---------------- 設定 ----------------
        static void SetEnabled(uint32_t effect, bool enabled);
        static bool IsEnabled(uint32_t effect);
        /// <summary>
        /// 自由な値 (index: 0〜7)。HLSLでは 0〜3番 = input.params[0].xyzw、4〜7番 = input.params[1].xyzw
        /// </summary>
        static void SetParam(uint32_t effect, uint32_t index, float value);
        /// <summary>自由な値を4つまとめて設定する。slot 0 = 0〜3番、slot 1 = 4〜7番</summary>
        static void SetParams(uint32_t effect, uint32_t slot, const Vector4& values);
        static float GetParam(uint32_t effect, uint32_t index);

        // ---------------- 描画 (エンジンから呼ばれる) ----------------
        /// <summary>
        /// PostProcess::Composite が書き込む先のRTV。
        /// 有効な効果があれば作業用テクスチャ、無ければゲーム画面を返す。Composite の中で1回だけ呼ぶこと
        /// </summary>
        static D3D12_CPU_DESCRIPTOR_HANDLE BeginComposite();
        /// <summary>登録した効果を順にかけて、ゲーム画面へ書く。Composite の後、2D描画の前に呼ぶ</summary>
        static void Draw();

        /// <summary>シェーダーに渡す経過時間を進める (RyoEngine::NewFrame から呼ばれる)</summary>
        static void NewFrame();

    private:
        static PostEffectManager* GetInstance();

        PostEffectManager() = default;
        ~PostEffectManager() = default;
        PostEffectManager(const PostEffectManager&) = delete;
        PostEffectManager& operator=(const PostEffectManager&) = delete;

        struct Effect {
            uint32_t id = kInvalidEffect;
            uint32_t shader = 0;
            bool enabled = true;
            Vector4 params[2] = { { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
        };

        // 作業用テクスチャ1枚分
        struct WorkTexture {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
            D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
            uint32_t textureHandle = 0; // TextureManager に登録したSRV
        };

        void CreateRootSignature();
        void CreateWorkTextures();
        void Transition(WorkTexture& texture, D3D12_RESOURCE_STATES newState);
        Effect* Find(uint32_t effect);
        // かける効果 (有効で、シェーダーがコンパイルできているもの) を集める
        void CollectActiveEffects(std::vector<const Effect*>& outEffects);
        // シェーダーごとのPSO。無ければ作る。シェーダーが使えなければ nullptr
        ID3D12PipelineState* GetPipelineState(uint32_t shader);
        // ShaderManager でシェーダーが再読み込みされたら、作ったPSOを捨てる (GPUが使い終わるまで数フレーム生かす)
        void RetirePipelinesIfShaderReloaded();

        std::vector<Effect> effects_; // 登録順 = かける順
        uint32_t nextId_ = 1;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        std::vector<uint8_t> vertexShaderBytecode_; // PostProcess/Composite.VS.hlsl (画面全体を覆う三角形)
        std::unordered_map<uint32_t, Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineCache_; // key = シェーダー
        struct RetiredPipeline {
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
            int framesLeft = 0;
        };
        std::vector<RetiredPipeline> retiredPipelines_;
        uint32_t shaderReloadCount_ = 0;
        // 捨てたPSOを、GPUが使い終わるまで生かしておくフレーム数
        int retireFrames_ = 3;

        std::array<WorkTexture, 2> workTextures_;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;

        // このフレームで Composite が作業用テクスチャ(0番)へ書いたか (BeginComposite で決まる)
        bool compositeToWork_ = false;
        // シェーダーに渡す経過時間 (秒)
        float time_ = 0.0f;
    };
}
