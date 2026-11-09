// Particle の頂点シェーダー。
// 頂点バッファは使わず、SV_VertexID(0〜5)から板ポリ(2枚の三角形)を作る。
// 寿命に合わせた色・大きさの変化、消え方(点滅・縮む・伸びる・色変化…)、速度方向への伸び、ジッターをここで行う。
#define PARTICLE_FX_RENDER
#include "Particle.hlsli"

StructuredBuffer<ParticleData> gParticles : register(t0);

static const float2 kCorners[6] =
{
    float2(-1.0f, 1.0f), float2(1.0f, 1.0f), float2(-1.0f, -1.0f),
    float2(-1.0f, -1.0f), float2(1.0f, 1.0f), float2(1.0f, -1.0f),
};

VertexShaderOutput main(uint vertexId : SV_VertexID, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;

    float2 corner = kCorners[vertexId];
    output.local = corner;
    output.texcoord = float2(corner.x * 0.5f + 0.5f, 0.5f - corner.y * 0.5f);

    ParticleData p = gParticles[instanceId];

    // 死んでいる粒は画面外の1点に潰す (面積0なのでピクセルシェーダーは走らない)
    if (p.remainingLife <= 0.0f)
    {
        output.position = float4(0.0f, 0.0f, -2.0f, 1.0f);
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }

    float lifeRatio = saturate(p.remainingLife / max(p.totalLife, 0.0001f));
    float age = 1.0f - lifeRatio;
    bool isChild = (p.flags & kFlagChild) != 0u;

    // 分裂の子は「縮みながらフェード」で消える
    uint death = isChild ? kDeathFade : gDeath;
    float k = DeathProgress(p.remainingLife, p.totalLife, isChild ? 0.6f : gDeathRatio);

    // --- 寿命に対する色・大きさの変化 ---
    float4 color = p.color * lerp(float4(1.0f, 1.0f, 1.0f, 1.0f), gEndColor, age);
    float scaleMul = lerp(gStartScale, gEndScale, age);
    if (isChild)
    {
        scaleMul *= 1.0f - k;
    }

    float alpha = 1.0f;
    float widthMul = 1.0f;
    float lengthMul = 1.0f;

    // --- 消え方 ---
    if (death == kDeathFade)
    {
        alpha *= 1.0f - k;
    }
    else if (death == kDeathBlink)
    {
        // 点いている時間の割合(duty)を 1 → 0 に減らしていく。周期と位相は粒ごとにばらばら
        float duty = 1.0f - k;
        float phase = frac(gTime * p.blinkFrequency + p.seed * 10.0f);
        alpha *= (phase < duty) ? 1.0f : 0.0f;
        alpha *= lerp(1.0f, 1.0f - k, gBlinkFade);
    }
    else if (death == kDeathShrink)
    {
        scaleMul *= 1.0f - k;
    }
    else if (death == kDeathStretch)
    {
        widthMul *= 1.0f - k;
        lengthMul *= 1.0f + k * gDeathStretch;
    }
    else if (death == kDeathColorShift)
    {
        // 元の色 → gDeathColor → 黒 (加算なら黒は見えなくなる)
        float3 shifted = (k < 0.5f)
            ? lerp(color.rgb, gDeathColor.rgb, k * 2.0f)
            : lerp(gDeathColor.rgb, float3(0.0f, 0.0f, 0.0f), (k - 0.5f) * 2.0f);
        color.rgb = shifted;
        alpha *= 1.0f - k * k;
    }
    else if (death == kDeathAbsorb)
    {
        scaleMul *= 1.0f - k * 0.9f;
    }
    // kDeathSplit : 弾ける瞬間まではそのまま見せる

    // --- 速いほど明るく ---
    float speed = length(p.velocity);
    color.rgb *= gIntensity * (1.0f + speed * gSpeedBrightness);
    color.a *= alpha;

    // --- 板の向き (速度方向に伸ばす or ロール回転) ---
    float2 screenVelocity = float2(dot(p.velocity, gCameraRight), dot(p.velocity, gCameraUp));
    float screenSpeed = length(screenVelocity);
    bool useStretch = (gStretch > 0.0f || death == kDeathStretch) && screenSpeed > 0.001f;

    float2 axisX;
    float2 axisY;
    if (useStretch)
    {
        axisX = screenVelocity / screenSpeed;
        axisY = float2(-axisX.y, axisX.x);
        lengthMul *= 1.0f + speed * gStretch;
    }
    else
    {
        float s = sin(p.rotation);
        float c = cos(p.rotation);
        axisX = float2(c, s);
        axisY = float2(-s, c);
    }

    float2 local = corner * p.scale * scaleMul;
    local.x *= lengthMul;
    local.y *= widthMul;
    float2 rotated = axisX * local.x + axisY * local.y;

    float3 worldPosition = p.position + gCameraRight * rotated.x + gCameraUp * rotated.y;

    // --- ジッター：一定間隔ごとに、粒ごとにばらばらの方向へカクッとずらす (電気っぽさ) ---
    if (gJitterStrength > 0.0f)
    {
        float jitterStep = floor(gTime * gJitterFrequency);
        float3 jitter = Hash31(p.seed * 113.0f + jitterStep * 7.77f) * 2.0f - 1.0f;
        worldPosition += jitter * gJitterStrength;
    }

    output.position = mul(float4(worldPosition, 1.0f), gViewProjection);
    output.color = color;
    return output;
}
