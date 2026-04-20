#include "Object3d.hlsli"

struct Material
{
    float4 color;
    int enableLghiting;
};
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSumpler : register(s0);

ConstantBuffer<DirectionalLight> gDirectionLight : register(b1);

struct PixelShaderOutput 
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) 
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSumpler, input.texcoord);
    
    if (gMaterial.enableLghiting != 0)
    {
        float cos = saturate(dot(normalize(input.normal), -gDirectionLight.direction));
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionLight.color.rgb * cos * gDirectionLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    return output;
}