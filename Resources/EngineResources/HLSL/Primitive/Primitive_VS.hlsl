#include "Primitive.hlsli"

struct ViewProjection
{
    float4x4 viewProjection;
};
ConstantBuffer<ViewProjection> gViewProjection : register(b0);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    // CPU側で既にワールド(または2Dスクリーン)座標まで変換済みなので、VP行列を掛けるだけでよい
    output.position = mul(input.position, gViewProjection.viewProjection);
    output.color = input.color;
    return output;
}
