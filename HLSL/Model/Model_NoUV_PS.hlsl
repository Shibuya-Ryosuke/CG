#include "Model_NoUV.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    int shadingMode;
    float2 padding;
    float4x4 uvTransform; // このシェーダーでは未使用。Model_PS.hlslのMaterialとメモリレイアウトを合わせるためだけに残す
};

// C++側 Light構造体(Light.h)と1:1でレイアウトを合わせること。全体で64byte。
// (Model_PS.hlslと全く同じ定義。共有ヘッダに切り出しても良い)
struct Light
{
    float4 color;
    float3 direction;
    float intensity;
    float3 position;
    float range;
    float spotAngle;
    float spotFalloff;
    uint type;
    float padding;
};

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT       = 1;
static const uint LIGHT_TYPE_SPOT        = 2;
static const uint LIGHT_TYPE_AREA        = 3;

ConstantBuffer<Material> gMaterial : register(b0);

// Light配列本体 (StructuredBuffer。C++側はRoot Descriptor SRVとして直接バインドしている)
// NOTE: このシェーダーはテクスチャ(t0)を使わないが、RootSignatureはModel_PS.hlslと共有のため
//       Light配列のレジスタ番号(t1)はModel_PS.hlsl側と揃えておく必要がある
StructuredBuffer<Light> gLights : register(t1);

cbuffer LightCountBuffer : register(b1)
{
    uint gLightCount;
};

cbuffer AmbientLightBuffer : register(b2)
{
    float4 gAmbientColor;
    float gAmbientIntensity;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

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

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャが無いので、Material.color(mtlのKd相当)をそのままベースカラーとして使う
    if (gMaterial.enableLighting != 0 && gMaterial.shadingMode != 0)
    {
        float3 normal = normalize(input.normal);
        float3 lightSum = float3(0.0f, 0.0f, 0.0f);

        for (uint i = 0; i < gLightCount; ++i)
        {
            Light light = gLights[i];

            if (light.type == LIGHT_TYPE_DIRECTIONAL)
            {
                float3 dirToLight = -light.direction;
                float cosTerm = ComputeCosTerm(normal, dirToLight, gMaterial.shadingMode);
                lightSum += light.color.rgb * cosTerm * light.intensity;
            }
            else
            {
                // NOTE: Areaは暫定的にPointと全く同じ計算にしている(Model_PS.hlsl側と同様)
                float3 toLightVec = light.position - input.worldPosition;
                float distance = length(toLightVec);
                float3 dirToLight = toLightVec / max(distance, 0.0001f);

                float attenuation = saturate(1.0f - (distance / max(light.range, 0.0001f)));
                attenuation *= attenuation;

                if (light.type == LIGHT_TYPE_SPOT)
                {
                    float cosAngle = dot(-dirToLight, normalize(light.direction));
                    float spotAttenuation = saturate((cosAngle - light.spotAngle) / max(light.spotFalloff, 0.0001f));
                    attenuation *= spotAttenuation;
                }

                float cosTerm = ComputeCosTerm(normal, dirToLight, gMaterial.shadingMode);
                lightSum += light.color.rgb * cosTerm * light.intensity * attenuation;
            }
        }

        lightSum += gAmbientColor.rgb * gAmbientIntensity;

        output.color.rgb = gMaterial.color.rgb * lightSum;
        output.color.a = gMaterial.color.a;
    }
    else
    {
        output.color = gMaterial.color;
    }

    return output;
}
