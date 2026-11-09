#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include "ParticleFxTypes.h"
#include "ParticleFxEmitter.h"

namespace App {

    /// <summary>
    /// 名前を付けたエフェクト (ParticleFxConfig + GPUバッファ) を管理し、
    /// ・ゲームから名前で再生する (Play / Stop)
    /// ・ImGuiのエディタで編集する (テンプレの適用、保存/読込)
    /// ・JSONの更新を見張って、自動で読み込み直す (ホットリロード)
    /// を行うクラス。
    ///
    /// Update / Draw / DrawImGui は Application が毎フレーム呼ぶので、シーンからは呼ばなくてよい。
    ///
    /// 使い方:
    ///   ParticleFxManager::Play("Burst", hitPosition);  // その場所で再生
    ///   ParticleFxManager::Play("Charge");              // 設定の位置 (追従キー or 基準座標) で再生
    ///   ParticleFxManager::Stop("Charge");              // 発生だけ止める (出ている粒は寿命まで残る)
    ///   ParticleFxManager::Clear("Charge");             // 出ている粒も消す
    /// </summary>
    class ParticleFxManager {
    public:
        static void Initialize();
        static void Finalize();

        /// <summary>発生・シミュレーションの予約 (Application::Draw から呼ぶ)</summary>
        static void Update();
        /// <summary>描画の予約 (Application::Draw から呼ぶ)</summary>
        static void Draw();
        /// <summary>エディタ (NRELEASEのみ。Update より前に呼ぶこと)</summary>
        static void DrawImGui();

        // --- 再生 ---
        static void Play(const std::string& name);
        static void Play(const std::string& name, const RyoEngine::Vector3& position);
        static void Stop(const std::string& name);
        static void Clear(const std::string& name);
        static void StopAll();
        static void ClearAll();
        static bool IsPlaying(const std::string& name);

        // --- 出ている数 (1フレーム前の値。分裂の子も含む) ---
        static uint32_t GetAliveCount(const std::string& name);
        static uint32_t GetMaxCount(const std::string& name);
        static uint32_t GetTotalAliveCount();
        static uint32_t GetTotalMaxCount();

        /// <summary>全エフェクトの最大数の合計の上限 (これを超える分は、作成・変更時に切り詰める)</summary>
        static uint32_t GetParticleBudget() { return GetInstance()->particleBudget_; }
        static void SetParticleBudget(uint32_t budget) { GetInstance()->particleBudget_ = budget; }

        // --- 作成・削除 ---
        static bool Create(const std::string& name, const ParticleFxConfig& config);
        static void Destroy(const std::string& name);
        static bool Contains(const std::string& name);
        static std::vector<std::string> GetNames();
        static ParticleFxConfig* GetConfig(const std::string& name);

        /// <summary>テンプレから、見本のエフェクト一式を作る (同名があれば作らない)</summary>
        static void CreateDefaultEffects();

        // --- 保存・読込 ---
        static void SetFolderPath(const std::string& path);
        static void Save();
        static bool Load();

    private:
        static ParticleFxManager* GetInstance();

        ParticleFxManager() = default;
        ~ParticleFxManager() = default;
        ParticleFxManager(const ParticleFxManager&) = delete;
        ParticleFxManager& operator=(const ParticleFxManager&) = delete;
        ParticleFxManager(ParticleFxManager&&) = delete;
        ParticleFxManager& operator=(ParticleFxManager&&) = delete;

        struct Effect {
            Effect() = default;
            ~Effect() = default;
            Effect(const Effect&) = delete;
            Effect& operator=(const Effect&) = delete;
            Effect(Effect&&) = delete;
            Effect& operator=(Effect&&) = delete;

            ParticleFxConfig config;
            ParticleFxEmitter emitter;

            bool playing = false;
            float elapsed = 0.0f;        // 再生してからの時間
            float time = 0.0f;           // ずっと進み続ける時間 (揺らぎ・点滅・ジッター用)
            float emitAccumulator = 0.0f;// 連続: 端数の持ち越し
            uint32_t burstsDone = 0;
            float burstTimer = 0.0f;
            bool hasPlayPosition = false;
            RyoEngine::Vector3 playPosition = { 0.0f, 0.0f, 0.0f };

            const bool* lastFlagPtr = nullptr;
            bool previousFlag = false;

            uint32_t textureHandle = 0;
            bool textureLoaded = false;
            std::string loadedTexturePath;
        };

        // 1体ぶん、GPUバッファ・テクスチャを設定に合わせる。
        // othersMaxTotal = ほかのエフェクトの最大数の合計 (予算を超える分は最大数を切り詰める)
        void ApplyConfig(Effect& effect, bool forceRecreate, uint32_t othersMaxTotal);
        // exclude 以外の全エフェクトの最大数の合計
        uint32_t SumMaxParticlesExcept(const Effect* exclude) const;
        void PlayImpl(Effect& effect, const RyoEngine::Vector3* position);
        void UpdateEffect(Effect& effect);
        void EmitParticles(Effect& effect, uint32_t count);

        RyoEngine::Vector3 ResolveOrigin(const Effect& effect) const;
        RyoEngine::Vector3 ResolveTarget(const Effect& effect, const RyoEngine::Vector3& origin) const;
        bool HasLiveTarget(const Effect& effect) const;
        RyoEngine::Vector3 ResolveShapeAxis(const ParticleFxConfig& config) const;
        RyoEngine::Vector3 ResolveMotionAxis(const ParticleFxConfig& config) const;
        float GetDeltaTime(const ParticleFxConfig& config) const;

        void SaveToFile(const std::string& fullPath);
        std::string GetFullPath() const { return folderPath_ + fileName_; }
        void CheckJsonHotReload(float deltaTime);
        void RememberJsonWriteTime();

#ifdef NRELEASE
        void DrawEffectEditor(const std::string& name, Effect& effect);
        // 出現数/上限数のゲージ (割合で 緑→黄→赤 に色が変わる)。width が負なら「残りの幅 x gaugeWidthRatio_」
        void DrawUsageGauge(uint32_t alive, uint32_t capacity, float width) const;
#endif

        std::map<std::string, std::unique_ptr<Effect>> effects_; // 名前順に並べたいので map
        std::mt19937 randomEngine_{ std::random_device{}() }; // 発生の命令に渡す乱数の種を作る

        // 1フレームで進める時間の上限 (ブレークポイントで止めた後などに、一気に飛ばないように)
        float maxDeltaTime_ = 0.1f;

        // 全エフェクトの最大数の合計の上限 (GPUメモリと毎フレームの処理量を抑えるため)
        uint32_t particleBudget_ = 1000000;

        // --- 保存・読込 ---
        std::string folderPath_ = "Resources/ApplicationResources/Json/";
        std::string fileName_ = "particle_fx.json";
        bool autoReloadJson_ = true;
        float jsonWatchInterval_ = 0.5f; // 何秒おきにJSONの更新を確認するか
        float jsonWatchTimer_ = 0.0f;
        std::filesystem::file_time_type jsonWriteTime_{};
        bool jsonExists_ = false;

        // --- エディタ ---
        std::string selectedName_;
        char newNameBuffer_[64] = {};
        int newTemplateIndex_ = 0;
        int applyTemplateIndex_ = 0;
        char texturePathBuffer_[256] = {};
        std::string textureBufferOwner_; // texturePathBuffer_ がどのエフェクトの値か

        // --- ゲージ ---
        float listNameWidth_ = 140.0f;      // 一覧の名前の幅 (この右にゲージ)
        float listHeight_ = 200.0f;         // 一覧の最初の高さ (下の枠線をドラッグで変えられる)
        float gaugeWidthRatio_ = 0.5f;      // ゲージの幅 = 残りの幅 x この割合
        float gaugeWarningRatio_ = 0.6f;    // ここで黄色になる
        float gaugeDangerRatio_ = 0.9f;     // ここで赤になる
        RyoEngine::Vector4 gaugeSafeColor_ = { 0.2f, 0.75f, 0.3f, 1.0f };
        RyoEngine::Vector4 gaugeWarningColor_ = { 0.9f, 0.8f, 0.15f, 1.0f };
        RyoEngine::Vector4 gaugeDangerColor_ = { 0.9f, 0.2f, 0.15f, 1.0f };
    };
}
