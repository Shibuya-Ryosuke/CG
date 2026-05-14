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
    output.worldPosition = mul(input.position, gTransformationMatrix.World).xyz;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    // ピクセルシェーダーでの投影サンプリング用に保持
    output.screenPosition = output.position; 
    
    return output;
}