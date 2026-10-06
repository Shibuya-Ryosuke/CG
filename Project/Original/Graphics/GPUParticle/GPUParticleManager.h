#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <cstdint>
#include "GPUParticleEmitter.h"

namespace RyoEngine {
    class Camera;

    // 1つのEmitterぶんの設定値。ImGuiでの編集対象であり、Save/Loadの対象でもある。
    struct GPUParticleEmitterConfig {
        std::string meshPath;
        std::string texturePath; // 空なら、meshPath(obj/mtl)本来のテクスチャを使う

        uint32_t maxParticleCount = 500;
        float scale = 1.0f;
        float lifeTime = 1.0f;

        // --- 速度 ---
        bool useRandomVelocity = false;
        Vector3 velocity = { 0.0f, 1.0f, 0.0f };       // useRandomVelocity=false時の固定値
        Vector3 velocityRandomMin = { -1.0f, 1.0f, -1.0f };
        Vector3 velocityRandomMax = { 1.0f, 2.0f, 1.0f };

        // --- 発生位置 ---
        // trackedPositionKey(マネージャーに登録された座標のキー)が登録済みであればその座標を基準にし、
        // 空または未登録ならbasePositionを基準にする。どちらの場合も、そこからの乱数オフセットを加える。
        std::string trackedPositionKey;
        Vector3 basePosition = { 0.0f, 0.0f, 0.0f };
        Vector3 positionOffsetRandomMin = { 0.0f, 0.0f, 0.0f };
        Vector3 positionOffsetRandomMax = { 0.0f, 0.0f, 0.0f };

        float gravity = 0.0f;
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        GPUParticleCommon::BlendMode blendMode = GPUParticleCommon::BlendMode::Additive;

        bool billboard = true;
        Vector3 fixedRotation = { 0.0f, 0.0f, 0.0f }; // billboard=false時、発生する全パーティクルに適用する固定姿勢

        // --- 発生条件 ---
        // conditionFlagKey(マネージャーに登録されたフラグのキー)が空または未登録なら、
        // TriggerEmit()で明示的に発生させる運用のみになる。
        std::string conditionFlagKey;
        bool continuousWhileTrue = true; // true:フラグが立っている間ずっと発生させ続ける / false:立ち上がった瞬間だけ1回
        float emitInterval = 0.1f;       // continuousWhileTrue時、何秒おきに発生させるか(0なら毎フレーム)
    };

    /// <summary>
    /// 名前を付けた複数のGPUParticleEmitterを生成・管理し、ImGuiから一括で編集できるようにするクラス。
    ///
    /// 「発生座標」「発生条件フラグ」は、コード側(BaseBulletやPlayer等)が持つ変数への
    /// 生ポインタを、マネージャー全体に対してキー名付きで登録してもらう方式にしている。
    /// 各Emitterは、登録済みのキーの中から好きなものを選んで参照する(Emitter名とは無関係)。
    /// 登録後は、対象が動いても再登録不要で毎フレーム自動的に参照しにいく。
    /// </summary>
    class GPUParticleManager {
    public:
        static GPUParticleManager* GetInstance();

        void Initialize();
        void Finalize();

        /// <summary>
        /// 新しいEmitterを名前付きで作成する。同名が既にあれば失敗しfalseを返す。
        /// </summary>
        bool CreateEmitter(const std::string& name, const GPUParticleEmitterConfig& config);
        void DestroyEmitter(const std::string& name);

        /// <summary>
        /// 追従対象の座標をキー名で登録する(コード側から呼ぶ)。Emitter名とは無関係。
        /// 同じキーで再登録すると上書きされる。posがnullptrなら登録解除と同じ。
        /// 対象オブジェクトを破棄する前にUnregisterPosition()を呼ぶこと。
        /// </summary>
        void RegisterPosition(const std::string& key, const Vector3* pos);
        void UnregisterPosition(const std::string& key);

        /// <summary>
        /// 発生条件として監視するフラグをキー名で登録する(コード側から呼ぶ)。Emitter名とは無関係。
        /// 同じキーで再登録すると上書きされる。flagがnullptrなら登録解除と同じ。
        /// 対象オブジェクトを破棄する前にUnregisterFlag()を呼ぶこと。
        /// </summary>
        void RegisterFlag(const std::string& key, const bool* flag);
        void UnregisterFlag(const std::string& key);

        /// <summary>
        /// コード側から、Emitterが使う座標/フラグのキーを切り替える(空文字で解除)。
        /// ImGuiで選ぶのが基本だが、コードから固定したい場合に使う。
        /// </summary>
        void SetTrackedPositionKey(const std::string& emitterName, const std::string& key);
        void SetConditionFlagKey(const std::string& emitterName, const std::string& key);

        /// <summary>
        /// 条件フラグに関係なく、このEmitterから1個だけ手動で発生させる。
        /// </summary>
        void TriggerEmit(const std::string& name);

        /// <summary>
        /// 全Emitterぶんまとめて、条件判定とUpdate()を行う。
        /// </summary>
        void Update(float deltaTime);

        /// <summary>
        /// 全Emitterぶんまとめて描画を予約する。
        /// </summary>
        void Draw(const Camera& camera);

        /// <summary>
        /// 新規作成フォーム・各Emitterの設定・保存/読込ボタンのImGuiパネルを描画する。
        /// </summary>
        void DrawImGui();

        void SetFolderPath(const std::string& path);
        void Save();
        void Load();

        // フルパスを取得するヘルパー関数
        std::string GetFullFilePath() const { return folderPath_ + kFileName; }

    private:
        struct ManagedEmitter {
            GPUParticleEmitterConfig config;
            GPUParticleEmitter emitter;

            const bool* lastFlagPtr = nullptr; // 前フレームに参照していたフラグ(参照先が変わったかの判定用。参照はしない)
            bool previousFlagState = false;
            float emitTimer = 0.0f;

            // ImGui入力用の安定バッファ(1文字入力するたびに他の処理が走らないよう、
            // Enter/フォーカス外れで確定するまではこちらに溜めておく)
            char meshPathBuffer[256] = {};
            char texturePathBuffer[256] = {};
        };

        // 1体ぶん、configの内容でEmitterを(再)生成する。パス変更時の再読み込みにも使う
        void ApplyConfig(ManagedEmitter& managed);
        // 1体ぶん、configの設定(固定/乱数の速度・座標オフセット等)に従って実際にEmit()を1回呼ぶ
        void EmitOnce(ManagedEmitter& managed);

        // キーから登録済みのポインタを引く。空キー・未登録ならnullptr
        const Vector3* FindPosition(const std::string& key) const;
        const bool* FindFlag(const std::string& key) const;

        std::unordered_map<std::string, std::unique_ptr<ManagedEmitter>> emitters_;

        // --- マネージャー全体の登録先(キー名 → コード側の変数へのポインタ) ---
        // ImGuiのコンボで名前順に並べたいのでstd::mapを使っている
        std::map<std::string, const Vector3*> positions_;
        std::map<std::string, const bool*> flags_;

        // --- 新規作成フォーム用の一時入力 ---
        char newNameBuffer_[64] = {};
        char newMeshPathBuffer_[256] = {};
        char newTexturePathBuffer_[256] = {};
        int newMaxParticleCount_ = 500;

        // --- 保存/読込 ---
        std::string folderPath_ = "Resources/ApplicationResources/TD2_1/Json/";
        char folderPathBuffer_[256] = {};
        static const std::string kFileName;

        // 実際にファイルへ保存を行う内部関数
        void SaveToFileInternal(const std::string& fullPath);

        // 上書き確認モーダル表示フラグ
        bool showOverwriteModal_ = false;
    };
}