// テスト用のカスタムポストエフェクト: 画面の端を暗くする (ビネット)
// 自由な値 (PostEffectManager::SetParam):
//   0番: 暗さの強さ (0〜1)
//   1番: 暗くなり始める位置 (画面中心からの距離。0.3 くらい)
//   2〜4番: 暗くする色 (R, G, B。0,0,0 で黒)
//
// 書き方の決まり (詳しくは Resources/EngineResources/HLSL/PostEffect/PostEffectCommon.hlsli の先頭):
//   ・PostEffect() は必須。SampleScene(uv) で今の画面の色を読み、加工して返す

// エディタ(IntelliSense)で型や関数が分かるようにするための include (無くてもコンパイルは通る)。
// パスはこのファイルから見た PostEffectCommon.hlsli の場所に合わせること
#include "../../HLSL/PostEffect/PostEffectCommon.hlsli"

float4 PostEffect(PostEffectInput input)
{
    float4 color = SampleScene(input.uv);

    // 画面の縦横比を合わせて、中心からの距離を測る (円形に暗くするため)
    float2 centered = (input.uv - 0.5f) * float2(input.screenSize.x / input.screenSize.y, 1.0f);
    float distance = length(centered);

    float strength = input.params[0].x;
    float start = input.params[0].y;
    float3 tint = float3(input.params[0].z, input.params[0].w, input.params[1].x);

    float amount = smoothstep(start, start + 0.6f, distance) * strength;
    color.rgb = lerp(color.rgb, tint, saturate(amount));
    return color;
}
