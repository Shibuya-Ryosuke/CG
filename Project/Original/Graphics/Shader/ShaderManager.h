#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace RyoEngine {

    /// <summary>
    /// アプリのカスタムシェーダー(モデル用)を読み込み・コンパイルして、ハンドルで管理するクラス。
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
    /// </summary>
    class ShaderManager {
    public:
        /// <summary>エンジン標準のシェーダー (カスタムシェーダーを指定していないモデル)</summary>
        static constexpr uint32_t kDefaultShader = 0;
        /// <summary>シェーダーごとの追加テクスチャの枚数 (HLSLの gShaderTexture0 / 1)</summary>
        static constexpr uint32_t kTextureSlotCount = 2;

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

        /// <summary>シェーダーごとの追加テクスチャを設定する (slot: 0 か 1)。0を渡すと white1x1 に戻る</summary>
        static void SetTexture(uint32_t shader, uint32_t slot, uint32_t textureHandle);
        /// <summary>追加テクスチャ。設定していなければ white1x1</summary>
        static uint32_t GetTexture(uint32_t shader, uint32_t slot);

        /// <summary>コンパイル済みのシェーダー。無効なハンドルやコンパイル失敗時はエンジン標準のもの</summary>
        static const ModelProgram& GetModelProgram(uint32_t shader);

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
            std::string name;          // 表示用
            std::string filePath;      // アプリのシェーダーのパス。空ならエンジン標準
            ModelProgram program;
            std::array<uint32_t, kTextureSlotCount> textures{}; // 0 = 未設定 (white1x1)
            bool failed = false;       // コンパイルに失敗して、エンジン標準で代わりに描いているか
            std::string error;         // 失敗したときのエラー文
        };

        // entry をコンパイルする。失敗したらエンジン標準のシェーダーで代わりにする
        void Compile(Entry& entry);
        // 1段階分(VS/PS/影)のコンパイル。失敗したら false
        bool CompileStage(const Entry& entry, const char* stageDefine, const char* mainFile, const wchar_t* profile,
            std::vector<uint8_t>& outBytecode, std::string& outError);

        std::vector<Entry> shaders_; // ハンドルが添字。0番はエンジン標準
        std::unordered_map<std::string, uint32_t> pathToHandle_;
        uint32_t reloadCount_ = 0;

        // #include を探すフォルダ (エンジンのシェーダーのフォルダ)
        std::wstring engineShaderDirectory_ = L"Resources/EngineResources/HLSL";
    };
}
