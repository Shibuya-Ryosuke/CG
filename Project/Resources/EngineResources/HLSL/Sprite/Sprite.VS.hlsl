#include "Sprite.hlsli"

struct SpriteTransformationMatrix
{
    float4x4 WVP;
};

ConstantBuffer<SpriteTransformationMatrix> gTransformationMatrix : register(b1);

struct SpriteVertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
};

SpriteVertexShaderOutput main(SpriteVertexShaderInput input)
{
    SpriteVertexShaderOutput output;
   
    output.position = mul(input.position, gTransformationMatrix.WVP);
    output.texcoord = input.texcoord;
    return output;
}