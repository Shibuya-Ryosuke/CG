// C++側 InstanceData構造体(InstancedModel.h)と1:1でレイアウトを合わせること
struct InstanceData
{
    float4x4 world;
    float4 color;
};

// インスタンス配列本体 (Root Descriptor SRV。DescriptorHeap登録は不要)
StructuredBuffer<InstanceData> gInstances : register(t0);

// カメラのView-Projection行列 (全インスタンス共通。1フレームに1回だけ更新される)
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

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float3 worldPosition : TEXCOORD1; // Point/Spot/AreaLightの計算に使う
};

// SV_InstanceID：このDrawInstanced呼び出しの中で、今何体目のインスタンスを処理しているか。
// 0, 1, 2, ... instances_.size()-1 の値がGPU側で自動的に渡されてくる。
VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    InstanceData instance = gInstances[instanceId];

    float4 worldPosition = mul(input.position, instance.world);
    output.position = mul(worldPosition, gViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.texcoord = input.texcoord;
    output.normal = normalize(mul(input.normal, (float3x3) instance.world));
    output.color = instance.color;

    return output;
}
