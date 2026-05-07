#include "Sprite.hlsli"

struct SpriteMaterial
{
    float4 color;
    float4x4 uvTransform;
};

ConstantBuffer<SpriteMaterial> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct SpritePixelShaderOutput
{
    float4 color : SV_TARGET0;
};

SpritePixelShaderOutput main(SpriteVertexShaderOutput input)
{
    SpritePixelShaderOutput output;
    
   
    float4 transformdUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformdUV.xy);
    
   
    output.color = textureColor;
    
    return output;
}