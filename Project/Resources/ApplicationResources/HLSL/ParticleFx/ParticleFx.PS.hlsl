// ParticleFx のピクセルシェーダー。
// 画像を使わず、板の中の位置(local: -1〜1)から形をプログラムで描く (gLook で切り替え)。
// ライティングは行わない。
#define PARTICLE_FX_RENDER
#include "ParticleFx.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

float4 main(VertexShaderOutput input) : SV_TARGET0
{
    if (gLook == kLookTexture)
    {
        return gTexture.Sample(gSampler, input.texcoord) * input.color;
    }

    float2 p = input.local;
    float r = length(p);
    float softness = max(gSoftness, 0.01f);
    float a;

    if (gLook == kLookSoftCircle)
    {
        // ふちがぼけた丸 (光の粒)
        a = pow(saturate(1.0f - r), softness);
    }
    else if (gLook == kLookSpark)
    {
        // ひし形 (火花。伸ばすと細い線になる)
        float d = abs(p.x) + abs(p.y);
        a = pow(saturate(1.0f - d), softness);
    }
    else if (gLook == kLookRing)
    {
        // 輪
        float ring = saturate(1.0f - abs(r - 0.7f) / 0.25f);
        a = pow(ring, softness);
    }
    else
    {
        // 十字の光芒 + 中心の丸 (星)
        float rays = saturate(1.0f - abs(p.x * p.y) * 12.0f) * saturate(1.0f - r);
        float core = pow(saturate(1.0f - r * 2.5f), 2.0f);
        a = saturate(pow(rays, softness) + core);
    }

    if (a <= 0.001f)
    {
        discard;
    }
    return float4(input.color.rgb, input.color.a * a);
}
