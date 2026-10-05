// C++側 GPUParticleData構造体(GPUParticleCommon.h)と1:1でレイアウトを合わせること。全体で64byte。
struct ParticleData
{
    float3 position;
    float  scale;
    float3 velocity;
    float  totalLife;
    float4 color;
    float3 rotation;
    float  remainingLife;
};

// パーティクル配列本体 (Root Descriptor SRV。DescriptorHeap登録は不要)
StructuredBuffer<ParticleData> gParticles : register(t0);

// カメラのView-Projection行列(このシェーダーではcameraRight/cameraUpは未使用)
cbuffer CameraBuffer : register(b0)
{
    float4x4 gViewProjection;
    float3   gCameraRight;
    float    padding0;
    float3   gCameraUp;
    float    padding1;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0; // このシェーダーでは未使用(InstancedMeshの頂点フォーマットに合わせているだけ)
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

// オイラー角(XYZ、ラジアン)から回転行列を作る。
// NOTE: 行ベクトル規約(v * M)・XYZの順で合成している。CPU側のMakeRotateMatrix()と
//       見た目が食い違う場合は、ここの合成順序を見直すこと。
float3x3 MakeRotationXYZ(float3 rotation)
{
    float sx = sin(rotation.x); float cx = cos(rotation.x);
    float sy = sin(rotation.y); float cy = cos(rotation.y);
    float sz = sin(rotation.z); float cz = cos(rotation.z);

    float3x3 rotX = float3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, cx, sx,
        0.0f, -sx, cx);
    float3x3 rotY = float3x3(
        cy, 0.0f, -sy,
        0.0f, 1.0f, 0.0f,
        sy, 0.0f, cy);
    float3x3 rotZ = float3x3(
        cz, sz, 0.0f,
        -sz, cz, 0.0f,
        0.0f, 0.0f, 1.0f);

    return mul(mul(rotX, rotY), rotZ);
}

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    ParticleData particle = gParticles[instanceId];

    float3x3 rotationMatrix = MakeRotationXYZ(particle.rotation);
    float3 localPosition = input.position.xyz * particle.scale;
    localPosition = mul(localPosition, rotationMatrix);
    float3 worldPosition = localPosition + particle.position;

    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.texcoord = input.texcoord;

    // 寿命末期(残り20%)になめらかにフェードアウトさせる
    float lifeRatio = saturate(particle.remainingLife / max(particle.totalLife, 0.0001f));
    float fade = smoothstep(0.0f, 0.2f, lifeRatio);
    output.color = float4(particle.color.rgb, particle.color.a * fade);

    return output;
}
