#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace RyoEngine {

    /// <summary>
    /// アプリのカスタムシェーダー(モデル用・ポストエフェクト用)を読み込み・コンパイルして、ハンドルで管理するクラス。
    /// TextureManager と同じく「1度だけ読み込み、ハンドルで渡す」。
    ///
    /// ・アプリのシェーダーファイルには Surface() (必須) と Vertex() (任意) だけを書く。
    ///   main はエンジン側の型紙 (Resources/EngineResources/HLSL/Model/ModelMain.*.hlsli) が持っている。
    ///   書き方は ModelSurface.hlsli の先頭を参照
    /// ・コンパイルに失敗したシェーダーは、エラーをログに出して「エンジン標準のシェーダー」で代わりに描く (止まらない)
    /// ・ReloadAll() / ImGuiの「Shader」ウィンドウのボタンで、実行中にコンパイルし直せる (ホットリロード)
    ///
    /// 使い方:
    ///   uint32_t dissolve = ShaderManager::Load("Dissolve");             // 名前でもパスでもよい
    ///   ShaderManager::SetTexture(dissolve, 0, RyoEngine::LoadTex("Noise"));
    ///   enemy->SetShader(dissolve);
    ///
    ///   uint32_t vignette = ShaderManager::LoadPostEffect("Vignette");  // ポストエフェクト用 (PostEffect() を書いたもの)
    ///   PostEffectManager::Add(vignette);
    /// </summary>
    class ShaderManager {
    public:
        /// <summary>エンジン標準のシェーダー (カスタムシェーダーを指定していないモデル)</summary>
        static constexpr uint32_t kDefaultShader = 0;
        /// <summary>シェーダーごとの追加テクスチャの枚数 (HLSLの gShaderTexture0 / 1)</summary>
        static constexpr uint32_t kTextureSlotCount = 2;

        /// <summary>シェーダーの種類 (書く関数と、使う型紙が違う)</summary>
        enum class Type {
            Model,      // Surface() / Vertex()。Model::SetShader で使う
            PostEffect, // PostEffect()。PostEffectManager::Add で使う
        };

        /// <summary>モデル描画に使う、コンパイル済みのシェーダー一式</summary>
        struct ModelProgram {
            std::vector<uint8_t> vertexShader;
            std::vector<uint8_t> pixelShader;
            std::vector<uint8_t> shadowVertexShader;
        };

        static void Initialize();
        static void Finalize();

        /// <summary>
        /// モデル用のカスタムシェーダーを読み込む。同じファイルなら読み込み済みのハンドルを返す。
        /// 名前 (拡張子なしのファイル名) でもパスでもよい。見つからなければ kDefaultShader。
        /// コンパイルに失敗した場合もハンドルは返す (直してホットリロードすれば反映される)
        /// </summary>
        static uint32_t Load(const std::string& nameOrPath);

        /// <summary>
        /// ポストエフェクト用のシェーダーを読み込む (Load と同じ決まり)。見つからなければ kDefaultShader (=使えない)
        /// </summary>
        static uint32_t LoadPostEffect(const std::string& nameOrPath);

        /// <summary>シェーダーごとの追加テクスチャを設定する (slot: 0 か 1)。0を渡すと white1x1 に戻る</summary>
        static void SetTexture(uint32_t shader, uint32_t slot, uint32_t textureHandle);
        /// <summary>追加テクスチャ。設定していなければ white1x1</summary>
        static uint32_t GetTexture(uint32_t shader, uint32_t slot);

        /// <summary>コンパイル済みのシェーダー。無効なハンドルやコンパイル失敗時はエンジン標準のもの</summary>
        static const ModelProgram& GetModelProgram(uint32_t shader);

        /// <summary>
        /// ポストエフェクト用のコンパイル済みピクセルシェーダー。
        /// ポストエフェクト用でない・コンパイルに失敗している場合は nullptr (その効果は飛ばされる)
        /// </summary>
        static const std::vector<uint8_t>* GetPostEffectPixelShader(uint32_t shader);

        /// <summary>表示用の名前 (拡張子なしのファイル名)。無効なハンドルなら空文字</summary>
        static const std::string& GetName(uint32_t shader);

        /// <summary>
        /// 読み込んだシェーダーを全部コンパイルし直す (ホットリロード)。
        /// 描画側 (ModelManager) は GetReloadCount() の変化を見て、PSOを作り直す
        /// </summary>
        static void ReloadAll();
        static uint32_t GetReloadCount() { return GetInstance()->reloadCount_; }

        /// <summary>シェーダー一覧・エラー・再読み込みボタンのImGuiウィンドウ (DEBUGのみ)</summary>
        static void DrawImGui();

    private:
        static ShaderManager* GetInstance();

        ShaderManager() = default;
        ~ShaderManager() = default;
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        struct Entry {
            Type type = Type::Model;
            std::string name;          // 表示用
            std::string filePath;      // アプリのシェーダーのパス。空ならエンジン標準
            ModelProgram program;
            std::vector<uint8_t> postEffectPixelShader; // Type::PostEffect のときだけ使う
            std::array<uint32_t, kTextureSlotCount> textures{}; // 0 = 未設定 (white1x1)
            bool failed = false;       // コンパイルに失敗して、エンジン標準で代わりに描いているか
            std::string error;         // 失敗したときのエラー文
        };

        // 読み込みの共通処理
        uint32_t LoadImpl(Type type, const std::string& nameOrPath);
        // entry をコンパイルする。失敗したら、モデル用はエンジン標準のシェーダーで代わりにする (ポストエフェクト用は飛ばす)
        void Compile(Entry& entry);
        void CompilePostEffect(Entry& entry);
        // 型紙をまとめた1つのソースをコンパイルする
        bool CompileSource(const std::string& userFile, const char* commonFile, const char* stageDefine, const char* mainFile,
            const wchar_t* profile, std::vector<uint8_t>& outBytecode, std::string& outError);
        // アプリのシェーダーを #include するためのパス (絶対パス、区切りは '/')
        static std::string ToIncludePath(const std::string& filePath);
        // 1段階分(VS/PS/影)のコンパイル。失敗したら false
        bool CompileStage(const Entry& entry, const char* stageDefine, const char* mainFile, const wchar_t* profile,
            std::vector<uint8_t>& outBytecode, std::string& outError);

        std::vector<Entry> shaders_; // ハンドルが添字。0番はエンジン標準
        std::unordered_map<std::string, uint32_t> pathToHandle_; // key = 種類 + パス
        uint32_t reloadCount_ = 0;

        // #include を探すフォルダ (エンジンのシェーダーのフォルダ)
        std::wstring engineShaderDirectory_ = L"Resources/EngineResources/HLSL";
    };
}
