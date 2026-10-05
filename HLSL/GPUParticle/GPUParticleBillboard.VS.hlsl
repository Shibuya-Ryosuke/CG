// C++側 GPUParticleData構造体(GPUParticleCommon.h)と1:1でレイアウトを合わせること。全体で64byte。
struct ParticleData
{
    float3 position;
    float  scale;
    float3 velocity;
    float  totalLife;
    float4 color;
    float3 rotation;       // ビルボードではzだけを「カメラ正面軸まわりのロール」として使う
    float  remainingLife;
};

// パーティクル配列本体 (Root Descriptor SRV。DescriptorHeap登録は不要)
StructuredBuffer<ParticleData> gParticles : register(t0);

cbuffer CameraBuffer : register(b0)
{
    float4x4 gViewProjection;
    float3   gCameraRight; // ワールド空間でのカメラの右方向
    float    padding0;
    float3   gCameraUp;    // ワールド空間でのカメラの上方向
    float    padding1;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0; // このシェーダーでは未使用
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    ParticleData particle = gParticles[instanceId];

    // ローカルXYを、rotation.z(ロール角)ぶんだけ2D回転させてから、
    // カメラの右・上ベクトルに展開する(奥行き方向の回転は「カメラを向く」性質上意味が無いので使わない)
    float roll = particle.rotation.z;
    float cs = cos(roll);
    float sn = sin(roll);

    float2 local = input.position.xy * particle.scale;
    float2 rotated = float2(
        local.x * cs - local.y * sn,
        local.x * sn + local.y * cs
    );

    float3 worldPosition = particle.position
        + gCameraRight * rotated.x
        + gCameraUp * rotated.y;

    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.texcoord = input.texcoord;

    // 寿命末期(残り20%)になめらかにフェードアウトさせる
    float lifeRatio = saturate(particle.remainingLife / max(particle.totalLife, 0.0001f));
    float fade = smoothstep(0.0f, 0.2f, lifeRatio);
    output.color = float4(particle.color.rgb, particle.color.a * fade);

    return output;
}
