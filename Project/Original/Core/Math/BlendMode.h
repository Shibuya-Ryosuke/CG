#pragma once

namespace RyoEngine {

    /// <summary>
    /// 描画ブレンドモードの種類
    /// </summary>
    enum class BlendMode {
        None,       // ブレンド無し（不透明・上書き）
        Normal,     // αブレンド（デフォルト）
        Add,        // 加算
        Subtract,   // 減算
        Multiply,   // 乗算
        Screen,     // スクリーン
    };

}