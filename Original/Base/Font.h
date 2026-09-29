#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <format>
#include "../Math/Math.h" // Vector2 などの定義がある場所（適宜調整してください）

namespace RyoEngine {

    // 1文字分のフォント配置情報
    struct FontChar {
        int32_t x = 0;         // 画像内の左上X座標
        int32_t y = 0;         // 画像内の左上Y座標
        int32_t width = 0;     // 文字の横幅
        int32_t height = 0;    // 文字の縦幅
        int32_t xoffset = 0;   // 描画時のX補正量
        int32_t yoffset = 0;   // 描画時のY補正量
        int32_t xadvance = 0;  // 次の文字への進み量
    };

    class Font {
    public:
        // 通常のコンストラクタ・デストラクタ
        Font() = default;
        ~Font() = default;

        // ★ コピーコンストラクタと代入演算子を明示的に削除
        Font(const Font&) = delete;            // コピーコンストラクタ削除
        Font& operator=(const Font&) = delete; // コピー代入演算子削除

        // 初期化用関数（.fntとテクスチャのロードを一括で行う）
        void Initialize(const std::string& fntFilePath, const std::string& textureFilePath);

        void Finalize();

        // 文字コード(Unicode)から文字情報を取得する関数
        const FontChar* GetCharInfo(char character) const;

        // 一行の高さ（改行時に使用）
        int32_t GetLineHeight() const { return lineHeight_; }

        
        
        // ★ 追加: 引数付きで scale を省略したい場合（デフォルト 1.0f）
        template <typename... Args>
        void ScreenPrint(std::format_string<Args...> fmt, Vector2 position, Args&&... args) {
#ifdef _DEBUG
            std::string formattedText = std::format(fmt, std::forward<Args>(args)...);
            RegisterText(formattedText, position, 1.0f);
#else
            fmt;position;(void(args), ...);
#endif
        }

        void DrawAllText();

    private:

        void CreateResource();

        void RegisterText(const std::string& text, Vector2 position, float scale);

        // .fntファイルを読み込む関数
        bool LoadFnt(const std::string& filePath);


        // 文字コードをキーにした連想配列
        std::unordered_map<wchar_t, FontChar> charMap_;
        int32_t lineHeight_ = 0; // 行の高さ

        uint32_t textureHandle_ = 0;


        static const size_t MAX_CHARS = 1024; // 最大描画文字数
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
        Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
        D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

        SpriteVertexData* vertexData_ = nullptr;
        uint32_t* indexData_ = nullptr;

        // マテリアル（UV変換行列を単位行列にするため）とWVP用
        Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
        Matrix4x4* wvpData_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
        SpriteMaterial* materialData_ = nullptr;

        // ScreenPrintで指定された文字列の情報を保持する構造体
        struct TextDrawCall {
            std::string text;
            Vector2 position;
            float scale;
        };

        // メンバ変数に追加
        std::vector<TextDrawCall> drawCalls_;
    };

}