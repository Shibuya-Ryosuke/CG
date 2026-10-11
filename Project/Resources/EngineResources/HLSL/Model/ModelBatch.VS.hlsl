#include "ModelInstance.hlsli"
#include "ModelBatch.hlsli"

// カメラのView-Projection (C++側 root param 0)
cbuffer CameraBuffer : register(b0)
{
    float4x4 gViewProjection;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    ModelInstance instance = gInstances[instanceId];

    float4 worldPosition = mul(input.position, instance.world);
    output.position = mul(worldPosition, gViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(input.normal, (float3x3) instance.world));

    // UV変換: uv' = uv * [m00 m01; m10 m11] + offset  (旧 mul(float4(uv,0,1), uvTransform) と同じ)
    float2 uv = input.texcoord;
    output.texcoord = float2(
        uv.x * instance.uvMat.x + uv.y * instance.uvMat.z,
        uv.x * instance.uvMat.y + uv.y * instance.uvMat.w) + instance.uvOffset;

    output.instanceId = instanceId;
    return output;
}
