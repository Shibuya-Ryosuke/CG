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
