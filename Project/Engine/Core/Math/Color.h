#pragma once
#include "Vector.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace RyoEngine {

    // =====================================================================
    // 色のユーティリティ
    //
    // ペイントソフトやWebのカラーコード (#FF8800 など) は「sRGB」という、人の目に合わせて
    // 暗い色を細かく表す決まりで書かれている。エンジンの描画 (ライティング・テクスチャ・ブルーム) は
    // 「リニア」(光の量に比例した値) で計算していて、最後に画面へ出すときに sRGB へ戻している。
    // なので、カラーコードの値をそのまま 0〜1 に割って渡すと、元の色より明るく白っぽくなる。
    // ColorFromHex は sRGB → リニアに自動で変換するので、カラーコードどおりの色で表示される。
    // (アルファは明るさではないので変換しない)
    // =====================================================================

    /// sRGB の値 (0〜1) をリニアに変換する
    inline float SRGBToLinear(float srgb) {
        if (srgb <= 0.04045f) {
            return srgb / 12.92f;
        }
        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    }

    /// リニアの値 (0〜1) を sRGB に変換する
    inline float LinearToSRGB(float linear) {
        if (linear <= 0.0031308f) {
            return linear * 12.92f;
        }
        return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }

    /// <summary>
    /// 0xRRGGBB の数値から色を作る (例: ColorFromHex(0xFF8800))。sRGB → リニアに自動で変換する。
    /// アルファは別に指定する (0〜1。変換しない)
    /// </summary>
    inline Vector4 ColorFromHex(uint32_t rgb, float alpha = 1.0f) {
        const float r = static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f;
        const float g = static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f;
        const float b = static_cast<float>(rgb & 0xFFu) / 255.0f;
        return { SRGBToLinear(r), SRGBToLinear(g), SRGBToLinear(b), alpha };
    }

    /// <summary>
    /// "#RRGGBB" / "#RRGGBBAA" / "#RGB" の文字列から色を作る (# は無くてもよい)。sRGB → リニアに自動で変換する。
    /// 読めない文字列のときは、間違いに気づけるようにマゼンタ (1, 0, 1, 1) を返す
    /// </summary>
    inline Vector4 ColorFromHex(std::string_view hex) {
        const Vector4 kInvalid = { 1.0f, 0.0f, 1.0f, 1.0f };
        if (!hex.empty() && hex.front() == '#') {
            hex.remove_prefix(1);
        }
        // 16進数の1文字を数にする (読めなければ -1)
        auto toNumber = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        uint32_t value = 0;
        for (char c : hex) {
            const int n = toNumber(c);
            if (n < 0) {
                return kInvalid;
            }
            value = (value << 4) | static_cast<uint32_t>(n);
        }

        if (hex.size() == 3) {
            // #RGB は #RRGGBB の省略 (例: #F80 = #FF8800)
            const uint32_t r = (value >> 8) & 0xFu;
            const uint32_t g = (value >> 4) & 0xFu;
            const uint32_t b = value & 0xFu;
            return ColorFromHex(((r * 17u) << 16) | ((g * 17u) << 8) | (b * 17u));
        }
        if (hex.size() == 6) {
            return ColorFromHex(value);
        }
        if (hex.size() == 8) {
            return ColorFromHex(value >> 8, static_cast<float>(value & 0xFFu) / 255.0f);
        }
        return kInvalid;
    }

    /// <summary>
    /// 0xRRGGBB の数値から、発光・ライト用の色 (Vector3) を作る。sRGB → リニアに自動で変換する
    /// </summary>
    inline Vector3 ColorFromHexRGB(uint32_t rgb) {
        const Vector4 c = ColorFromHex(rgb);
        return { c.x, c.y, c.z };
    }
    /// <summary>"#RRGGBB" の文字列から、発光・ライト用の色 (Vector3) を作る (アルファは無視)</summary>
    inline Vector3 ColorFromHexRGB(std::string_view hex) {
        const Vector4 c = ColorFromHex(hex);
        return { c.x, c.y, c.z };
    }

    /// <summary>
    /// 色 (リニア) を "#RRGGBBAA" の文字列にする (ImGui に表示したり、ログに出したりするとき用)。
    /// withAlpha が false なら "#RRGGBB"
    /// </summary>
    inline std::string ColorToHex(const Vector4& linearColor, bool withAlpha = true) {
        auto toByte = [](float v) -> unsigned int {
            const float clamped = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            return static_cast<unsigned int>(clamped * 255.0f + 0.5f);
        };
        char buffer[16] = {};
        const unsigned int r = toByte(LinearToSRGB(linearColor.x));
        const unsigned int g = toByte(LinearToSRGB(linearColor.y));
        const unsigned int b = toByte(LinearToSRGB(linearColor.z));
        if (withAlpha) {
            std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", r, g, b, toByte(linearColor.w));
        } else {
            std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", r, g, b);
        }
        return buffer;
    }
}
