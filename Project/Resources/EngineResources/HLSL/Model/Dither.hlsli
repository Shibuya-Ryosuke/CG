#ifndef DITHER_HLSLI
#define DITHER_HLSLI

// カメラに近すぎるピクセルをディザ(スクリーンドア透明)で消すための共通処理。
// Model_PS / Model_NoUV_PS / Instanced_PS から #include して使う。
// 頂点シェーダーや定数バッファの変更は不要(SV_Positionだけで完結する)。

// --- 調整用の距離 (ビュー空間のZ深度 = カメラからの奥行き距離、ワールド単位) ---
// C++側 ModelManager::SetNearFadeStart / SetNearFadeEnd で設定し、ルート定数で渡される
//   fadeStart : この距離以上では完全に不透明。ここから近づくほど薄くなり始める
//   fadeEnd   : この距離以下では完全に消える。カメラのニアクリップ距離より少し大きい値にすること

// 4x4 ベイヤー行列 (0〜15)。ピクセル座標%4で参照して、ピクセルごとの閾値にする
static const float kBayer4x4[16] =
{
     0.0f,  8.0f,  2.0f, 10.0f,
    12.0f,  4.0f, 14.0f,  6.0f,
     3.0f, 11.0f,  1.0f,  9.0f,
    15.0f,  7.0f, 13.0f,  5.0f
};

// カメラからの距離 → 不透明度 (0:完全に消える 〜 1:完全に描く)
// svPosition : ピクセルシェーダー入力の SV_Position (input.position をそのまま渡す)
// NOTE: PS入力のSV_Position.wは「1 / クリップ空間のw」になっている。
//       透視投影ではクリップ空間のw = ビュー空間のZ深度なので、逆数を取れば距離になる。
//       (平行投影ではwが常に1になるため、この方式は使えない)
float ComputeNearCameraOpacity(float4 svPosition, float fadeStart, float fadeEnd)
{
    float viewDepth = svPosition.w;
    return saturate((viewDepth - fadeEnd) / max(fadeStart - fadeEnd, 0.0001f));
}

// 不透明のモデル用。ピクセルシェーダーの先頭で呼ぶ。カメラに近いほど、パターン状にピクセルをdiscardする。
// opacity : ComputeNearCameraOpacity の結果
// (半透明・加算などはディザではなく、ModelBatch.PS で色・アルファを薄くして消す)
void ApplyNearCameraDither(float4 svPosition, float opacity)
{
    // 十分遠ければ何もしない(ほとんどのピクセルはここで抜ける)
    if (opacity >= 1.0f)
    {
        return;
    }

    // ピクセル座標からベイヤー閾値(0〜1)を作る
    uint2 pixel = (uint2) svPosition.xy % 4;
    float threshold = (kBayer4x4[pixel.y * 4 + pixel.x] + 0.5f) / 16.0f;

    // 不透明度が閾値に届かないピクセルは捨てる (opacity - threshold < 0 でdiscard)
    clip(opacity - threshold);
}

#endif // DITHER_HLSLI
