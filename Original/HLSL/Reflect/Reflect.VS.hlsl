#include "Reflect.hlsli"

struct TransformationMatrixForReflect 
{
    float4x4 WVP;
    float4x4 World;
    float4x4 ReflectVP;
};

ConstantBuffer<TransformationMatrixForReflect> gTransformationMatrix : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.position = mul(input.position, gTransformationMatrix.WVP);
    float4 worldPos = mul(input.position, gTransformationMatrix.World);
    output.worldPosition = worldPos.xyz;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    // 修正コード：ReflectVPを掛けたあと、その場で w で割り算（透視除算）まで終わらせる
    float4 projPos = mul(worldPos, gTransformationMatrix.ReflectVP);
    
    // w が 0 になるのを防ぐガード（一応）
    if (projPos.w == 0.0f)
    {
        projPos.w = 0.0001f;
    }
    
    // XYをWで割り、あらかじめ 0 〜 1 のUV空間に変換してしまう（ZとWも一応そのまま送る）
    output.screenPosition.xy = projPos.xy / projPos.w;
    output.screenPosition.z = projPos.z;
    output.screenPosition.w = projPos.w;
    return output;
}