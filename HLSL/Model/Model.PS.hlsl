#include "Model.hlsli"

struct Material
{
    float4 color;
    int enableLighting;
    int shadingMode;
    float2 padding;
    float4x4 uvTransform;
};
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<DirectionalLight> gDirectionLight : register(b1);

struct PixelShaderOutput 
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) 
{

    
    PixelShaderOutput output;
    float4 transformdUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformdUV.xy);
    
    if (gMaterial.enableLighting != 0)
    {
        float cos = 0.0f;
        switch (gMaterial.shadingMode)
        {
            case 0:
             // Lambert
            cos = saturate(dot(normalize(input.normal), -gDirectionLight.direction));
                break;
            
            case 1:
             // Half Lambert
             float NdotL = dot(normalize(input.normal), -gDirectionLight.direction);
                cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
             break;
        }
        
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionLight.color.rgb * cos * gDirectionLight.intensity;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        output.color = gMaterial.color * textureColor;
    }
    return output;
}