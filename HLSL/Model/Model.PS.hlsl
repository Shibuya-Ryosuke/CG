#include "Model.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    int shadingMode;
    float2 padding;
    float4x4 uvTransform;
};

// C++側 Light構造体(Light.h)と1:1でレイアウトを合わせること。全体で64byte。
struct Light
{
    float4 color; // rgb=色, a=未使用
    float3 direction; // Directional/Spotで使用（正規化済み）
    float intensity; // 共通：明るさ
    float3 position; // Point/Spot/Areaで使用
    float range; // Point/Spotの減衰距離
    float spotAngle; // Spotの照射角
    float spotFalloff; // Spotの縁の滑らかさ
    uint type; // ライト種別 (0:Directional 1:Point 2:Spot 3:Area)
    float padding; // 16byteアライン調整用
};

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
// static const uint LIGHT_TYPE_POINT       = 1; // 未実装
// static const uint LIGHT_TYPE_SPOT        = 2; // 未実装
// static const uint LIGHT_TYPE_AREA        = 3; // 未実装

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// Light配列本体 (StructuredBuffer。C++側はRoot Descriptor SRVとして直接バインドしている)
StructuredBuffer<Light> gLights : register(t1);

// 現在の有効ライト数
cbuffer LightCountBuffer : register(b1)
{
    uint gLightCount;
};

// アンビエントライト(シーン全体への底上げ光。方向・位置を持たない)
cbuffer AmbientLightBuffer : register(b2)
{
    float4 gAmbientColor;
    float gAmbientIntensity;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformdUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformdUV.xy);

    clip(textureColor.a * gMaterial.color.a * input.alpha - 0.3f);

    // NOTE: 以前はshadingMode(0:None/1:Lambert/2:HalfLambert)をswitchで分岐し、
    //       Noneの場合はライト計算をせず即returnしていた。
    //       「ライティング無効(enableLighting==0)」の場合の処理と実質同じ式だったため、
    //       ここでは「ライティングを計算するか(enableLighting!=0 かつ shadingMode!=0)」と
    //       「計算しないか」の2分岐に整理している。
    if (gMaterial.enableLighting != 0 && gMaterial.shadingMode != 0)
    {
        float3 normal = normalize(input.normal);
        float3 lightSum = float3(0.0f, 0.0f, 0.0f);

        // シーン内の全ライトをループして加算する
        for (uint i = 0; i < gLightCount; ++i)
        {
            Light light = gLights[i];

            // NOTE: 現状はDirectionalLightのみ計算に対応。Point/Spot/Areaは今後実装。
            if (light.type != LIGHT_TYPE_DIRECTIONAL)
            {
                continue;
            }

            float cos = 0.0f;
            if (gMaterial.shadingMode == 1)
            {
                // Lambert
                cos = saturate(dot(normal, -light.direction));
            }
            else if (gMaterial.shadingMode == 2)
            {
                // Half Lambert
                float NdotL = dot(normal, -light.direction);
                cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            }

            lightSum += light.color.rgb * cos * light.intensity;
        }

        // アンビエント(間接光の簡易近似)を最後に加算する
        lightSum += gAmbientColor.rgb * gAmbientIntensity;

        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * lightSum;
        output.color.a = gMaterial.color.a * textureColor.a * input.alpha;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
        output.color.a *= input.alpha;
    }
    return output;
}
