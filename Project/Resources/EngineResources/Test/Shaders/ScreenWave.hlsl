// テスト用のカスタムポストエフェクト: 画面を波打たせる (SampleScene をずらして読む例)
// 自由な値 (PostEffectManager::SetParam):
//   0番: 揺れの大きさ (UV。0.005 くらい)
//   1番: 波の細かさ (20 くらい)
//   2番: 波の速さ (3 くらい)

// エディタ(IntelliSense)用の include (無くてもコンパイルは通る)
#include "../../HLSL/PostEffect/PostEffectCommon.hlsli"

float4 PostEffect(PostEffectInput input)
{
    float amplitude = input.params[0].x;
    float frequency = input.params[0].y;
    float speed = input.params[0].z;

    // 縦の位置に応じて、横にずらして読む
    float2 uv = input.uv;
    uv.x += sin(uv.y * frequency + input.time * speed) * amplitude;
    return SampleScene(uv);
}
