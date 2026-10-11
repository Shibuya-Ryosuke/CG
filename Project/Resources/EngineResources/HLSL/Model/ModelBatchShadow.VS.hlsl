#include "ModelInstance.hlsli"

// ライトのView-Projection (C++側 root param 6)
cbuffer LightViewProjBuffer : register(b3)
{
    float4x4 gLightViewProj;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    float4 worldPosition = mul(input.position, gInstances[instanceId].world);
    output.position = mul(worldPosition, gLightViewProj);
    return output;
}
