#include "Model_NoUV.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    int shadingMode;
    float2 padding;
    float4x4 uvTransform; // このシェーダーでは未使用。Model_PS.hlslのMaterialとメモリレイアウトを合わせるためだけに残す
};
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionLight : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // テクスチャが無いので、Material.color(mtlのKd相当)をそのままベースカラーとして使う
    if (gMaterial.enableLighting != 0)
    {
        float cos = 0.0f;
        switch (gMaterial.shadingMode)
        {
            case 0:
                // None
                cos = 1.0f;
                output.color.rgb = gMaterial.color.rgb;
                output.color.a = gMaterial.color.a;
                return output;
                break;

            case 1:
                // Lambert
                cos = saturate(dot(normalize(input.normal), -gDirectionLight.direction));
                break;

            case 2:
                // Half Lambert
                float NdotL = dot(normalize(input.normal), -gDirectionLight.direction);
                cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
                break;
        }

        output.color.rgb = gMaterial.color.rgb * gDirectionLight.color.rgb * cos * gDirectionLight.intensity;
        output.color.a = gMaterial.color.a;
    }
    else
    {
        output.color = gMaterial.color;
    }

    return output;
}
