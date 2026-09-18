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
    float4 color;       // rgb=色, a=未使用
    float3 direction;   // Directional/Spotで使用（正規化済み）
    float intensity;    // 共通：明るさ
    float3 position;    // Point/Spot/Areaで使用
    float range;        // Point/Spotの減衰距離
    float spotAngle;    // Spotの照射角 (cos値)
    float spotFalloff;  // Spotの縁の滑らかさ
    uint type;          // ライト種別 (0:Directional 1:Point 2:Spot 3:Area)
    float padding;      // 16byteアライン調整用
};

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT       = 1;
static const uint LIGHT_TYPE_SPOT        = 2;
static const uint LIGHT_TYPE_AREA        = 3;

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

// ライトのView-Projection行列 (シャドウマップ参照用。C++側のroot param 6、Vertex/Pixel共通)
cbuffer LightViewProjBuffer : register(b3)
{
    float4x4 gLightViewProj;
};

// シャドウマップ本体と、PCF用の比較サンプラー
Texture2D<float> gShadowMap : register(t2);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelShaderOutput 
{
    float4 color : SV_TARGET0;
};

// 法線とライトへの向き(dirToLight：ピクセルからライトの方向)から、Lambert/HalfLambertの係数を計算する
float ComputeCosTerm(float3 normal, float3 dirToLight, int shadingMode)
{
    if (shadingMode == 1)
    {
        // Lambert
        return saturate(dot(normal, dirToLight));
    }
    else if (shadingMode == 2)
    {
        // Half Lambert
        float NdotL = dot(normal, dirToLight);
        return pow(NdotL * 0.5f + 0.5f, 2.0f);
    }
    return 1.0f;
}

// ワールド座標から、シャドウマップを使って「影の量」を計算する (1:影なし 〜 0:完全に影)
// NOTE: 現状はDirectionalLight(Light 0)専用。Point/Spotの影は別方式が必要なため未対応。
float CalculateShadowFactor(float3 worldPosition)
{
    float4 lightClipPos = mul(float4(worldPosition, 1.0f), gLightViewProj);
    // 正射影なのでw除算は本来不要だが、念のため行っておく
    float3 lightNDC = lightClipPos.xyz / lightClipPos.w;

    // NDC(-1〜1、Yは上向き)からテクスチャUV(0〜1、Vは下向き)へ変換
    float2 shadowUV;
    shadowUV.x = lightNDC.x * 0.5f + 0.5f;
    shadowUV.y = -lightNDC.y * 0.5f + 0.5f;
    float currentDepth = lightNDC.z;

    // シャドウマップの範囲外(影を落とす対象範囲の外)は、影なし扱いにする
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f ||
        currentDepth < 0.0f || currentDepth > 1.0f)
    {
        return 1.0f;
    }

    // SampleCmpLevelZeroでハードウェアPCF(2x2の平均)を使う。
    // ComparisonFunc=LESS_EQUALなので、currentDepth <= シャドウマップの深度 なら1(影なし)に近づく。
    return gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth);
}

PixelShaderOutput main(VertexShaderOutput input) 
{
    PixelShaderOutput output;
    float4 transformdUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformdUV.xy);

    clip(textureColor.a * gMaterial.color.a * input.alpha - 0.3f);

    if (gMaterial.enableLighting != 0 && gMaterial.shadingMode != 0)
    {
        float3 normal = normalize(input.normal);
        float3 lightSum = float3(0.0f, 0.0f, 0.0f);

        for (uint i = 0; i < gLightCount; ++i)
        {
            Light light = gLights[i];

            if (light.type == LIGHT_TYPE_DIRECTIONAL)
            {
                // Directional: 光源からピクセルへ向かう向きが light.direction なので、
                // ピクセルから見た「ライトへの向き」はその逆
                float3 dirToLight = -light.direction;
                float cosTerm = ComputeCosTerm(normal, dirToLight, gMaterial.shadingMode);
                float shadow = CalculateShadowFactor(input.worldPosition);
                lightSum += light.color.rgb * cosTerm * light.intensity * shadow;
            }
            else
            {
                // Point / Spot / Area共通：ピクセルからライトへの向きと距離を計算
                // NOTE: Areaは暫定的にPointと全く同じ計算にしている。
                //       本来の面光源(サイズを持つ発光面からの光)を再現するには、
                //       Light構造体へのサイズ/形状情報の追加と、LTC等の専用手法が別途必要。
                float3 toLightVec = light.position - input.worldPosition;
                float distance = length(toLightVec);
                float3 dirToLight = toLightVec / max(distance, 0.0001f);

                // 距離による減衰 (0〜rangeで1→0に落ちる簡易モデル)
                float attenuation = saturate(1.0f - (distance / max(light.range, 0.0001f)));
                attenuation *= attenuation;

                if (light.type == LIGHT_TYPE_SPOT)
                {
                    // スポットのコーン減衰：light.directionは「スポット自体が照らす向き」
                    float cosAngle = dot(-dirToLight, normalize(light.direction));
                    float spotAttenuation = saturate((cosAngle - light.spotAngle) / max(light.spotFalloff, 0.0001f));
                    attenuation *= spotAttenuation;
                }

                float cosTerm = ComputeCosTerm(normal, dirToLight, gMaterial.shadingMode);
                lightSum += light.color.rgb * cosTerm * light.intensity * attenuation;
            }
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
