#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "../Math/Math.h" // Vector2 などの定義がある場所（適宜調整してください）
#include "../2D/Sprite.h"

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

    class FontLoader {
    public:
        // 通常のコンストラクタ・デストラクタ
        FontLoader() = default;
        ~FontLoader() = default;

        // ★ コピーコンストラクタと代入演算子を明示的に削除
        FontLoader(const FontLoader&) = delete;            // コピーコンストラクタ削除
        FontLoader& operator=(const FontLoader&) = delete; // コピー代入演算子削除

        // 初期化用関数（.fntとテクスチャのロードを一括で行う）
        bool Initialize(const std::string& fntFilePath, const std::string& textureFilePath);

        // 文字コード(Unicode)から文字情報を取得する関数
        const FontChar* GetCharInfo(char character) const;

        // 一行の高さ（改行時に使用）
        int32_t GetLineHeight() const { return lineHeight_; }

        void ScreenPrint(const std::string& text, Vector2 position, float scale = 1.0f);

    private:
        // .fntファイルを読み込む関数
        bool LoadFnt(const std::string& filePath);


        // 文字コードをキーにした連想配列
        std::unordered_map<wchar_t, FontChar> charMap_;
        int32_t lineHeight_ = 0; // 行の高さ

        uint32_t textureHandle_ = 0;
        std::unique_ptr<Sprite> fontSprite_;
    };

}