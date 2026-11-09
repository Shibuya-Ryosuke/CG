// ParticleFx のシミュレーション本体。
// 生きている粒は「動き方(gMotion)」に従って1フレーム進める。
// 死んでいる粒は、発生の命令 → 前フレームの分裂の子 の順に、空きスロットとして引き取る。
// 発生の命令から引き取った粒は、ここで初期値(位置・速度・色…)を乱数で作る (CPUは1粒ずつ作らない)。
#define PARTICLE_FX_SIMULATION
#include "ParticleFx.hlsli"

RWStructuredBuffer<FxParticle> gParticles : register(u0); // パーティクル本体
RWStructuredBuffer<uint> gCounters : register(u1);         // [0]配った数 [1][2]分裂の子の数 [3]生きている数
RWStructuredBuffer<FxParticle> gChildren : register(u2);   // 分裂の子のリスト (gMaxCount * 2 件。前半/後半を交互に使う)
StructuredBuffer<SpawnBatch> gBatches : register(t0);      // 今フレームの発生の命令

// 発生の命令 batch の localIndex 番目の粒を作る (C++側でやっていた初期値の計算と同じ)
FxParticle CreateParticle(SpawnBatch batch, uint localIndex)
{
    uint state = PcgHash(batch.seed ^ PcgHash(localIndex + 0x9E3779B9u));

    // --- 発生位置 (origin からのずれ) と、中心から外への向き ---
    float3 offset = float3(0.0f, 0.0f, 0.0f);
    float3 direction = float3(0.0f, 1.0f, 0.0f);
    float inner = saturate(gInnerRatio);
    float radius = gRadius * batch.radiusScale;

    if (gShape == kShapeSphere)
    {
        direction = RandomUnitVector(state);
        // 体積で均等になるよう3乗根を取る
        float u = RandomRange(state, inner * inner * inner, 1.0f);
        offset = direction * (radius * pow(u, 1.0f / 3.0f));
    }
    else if (gShape == kShapeRing || gShape == kShapeCylinder)
    {
        float angle = NextRandom(state) * kTwoPi;
        direction = batch.tangent1 * cos(angle) + batch.tangent2 * sin(angle);
        // 面積で均等になるよう平方根を取る
        float u = RandomRange(state, inner * inner, 1.0f);
        offset = direction * (radius * sqrt(u));
        if (gShape == kShapeCylinder)
        {
            offset += batch.axis * RandomRange(state, -gHeight * 0.5f, gHeight * 0.5f);
        }
    }
    else if (gShape == kShapeLine)
    {
        offset = gLineEnd * NextRandom(state);
        direction = RandomUnitVector(state);
    }
    else if (gShape == kShapeBox)
    {
        offset = RandomInBox(state, -gBoxSize * 0.5f, gBoxSize * 0.5f);
        float offsetLength = length(offset);
        if (offsetLength > 0.0001f)
        {
            direction = offset / offsetLength;
        }
        else
        {
            direction = RandomUnitVector(state);
        }
    }
    else
    {
        direction = RandomUnitVector(state); // 点
    }

    // --- 初速 ---
    float speed = RandomRange(state, gSpeedMin, gSpeedMax);
    float3 velocity = float3(0.0f, 0.0f, 0.0f);
    if (gMotion == kMotionExplode || gVelocityMode == kVelocityRadial)
    {
        velocity = direction * speed;
    }
    else if (gVelocityMode == kVelocityFixed)
    {
        velocity = gVelocity;
    }
    else if (gVelocityMode == kVelocityTangent)
    {
        float3 swirl = cross(batch.axis, direction);
        float swirlLength = length(swirl);
        if (swirlLength > 0.0001f)
        {
            velocity = swirl / swirlLength * speed;
        }
    }
    else
    {
        velocity = RandomInBox(state, gVelocityMin, gVelocityMax);
    }

    // --- 1粒ぶんのデータ ---
    FxParticle p;
    p.position = batch.origin + offset;
    p.scale = RandomRange(state, gScaleMin, gScaleMax);
    p.velocity = velocity;
    float life = max(RandomRange(state, gLifeMin, gLifeMax), 0.01f);
    p.totalLife = life;
    p.color = lerp(gColorA, gColorB, NextRandom(state));
    // 収束は「目標からの相対位置」、カーブは「発生位置」を覚えておく
    p.start = (gMotion == kMotionConverge) ? (p.position - batch.target) : p.position;
    p.remainingLife = life;
    p.control = float3(0.0f, 0.0f, 0.0f);
    if (gMotion == kMotionBezier)
    {
        p.control = (p.position + batch.target) * 0.5f + RandomInBox(state, gControlOffsetMin, gControlOffsetMax);
    }
    p.seed = NextRandom(state);
    p.target = batch.target;
    p.rotation = 0.0f;
    if (gRandomRotation != 0u)
    {
        p.rotation = NextRandom(state) * kTwoPi;
    }
    p.rotationSpeed = RandomRange(state, gRotationSpeedMin, gRotationSpeedMax);
    p.blinkFrequency = RandomRange(state, gBlinkFrequencyMin, gBlinkFrequencyMax);
    p.flags = 0u;
    p.padding = 0.0f;
    return p;
}

// 物理で動かす (重力・空気抵抗・揺らぎ・床で跳ねる)
void SimulatePhysics(inout FxParticle p, float dt)
{
    p.velocity.y -= gGravity * dt;
    p.velocity *= exp(-gDrag * dt);
    if (gNoiseStrength > 0.0f)
    {
        p.velocity += FlowField(p.position * gNoiseFrequency + p.seed * 7.0f, gTime) * gNoiseStrength * dt;
    }
    p.position += p.velocity * dt;

    if (gBounce != 0u && p.position.y < gFloorY && p.velocity.y < 0.0f)
    {
        p.position.y = gFloorY;
        p.velocity.y = -p.velocity.y * gRestitution;
        p.velocity.x *= gFriction;
        p.velocity.z *= gFriction;
    }
}

// 寿命が尽きた瞬間に、小さな粒を gSplitCount 個、次のフレーム用のリストへ書き出す
void SpawnChildren(FxParticle p)
{
    uint writeSlot = 1u - gChildReadSlot;
    uint base;
    InterlockedAdd(gCounters[1u + writeSlot], gSplitCount, base);

    [loop]
    for (uint i = 0u; i < gSplitCount; ++i)
    {
        uint childIndex = base + i;
        if (childIndex >= gMaxCount)
        {
            break; // リストが一杯なら諦める
        }

        float fi = (float)i;
        float3 h = Hash31(p.seed * 91.7f + fi * 13.1f);
        float3 dir = normalize(h * 2.0f - 1.0f + float3(0.0001f, 0.0002f, 0.0003f));

        FxParticle c;
        c.position = p.position;
        c.scale = p.scale * gSplitScale;
        c.velocity = dir * gSplitSpeed * (0.6f + 0.4f * h.x) + p.velocity * 0.2f;
        float life = gSplitLife * (0.7f + 0.6f * Hash11(p.seed * 17.3f + fi));
        c.totalLife = life;
        c.color = p.color;
        c.start = p.position;
        c.remainingLife = life;
        c.control = p.control;
        c.seed = frac(p.seed * 7.13f + fi * 0.618f);
        c.target = p.target;
        c.rotation = p.rotation;
        c.rotationSpeed = p.rotationSpeed;
        c.blinkFrequency = p.blinkFrequency;
        c.flags = kFlagChild;
        c.padding = 0.0f;

        gChildren[writeSlot * gMaxCount + childIndex] = c;
    }
}

void Simulate(inout FxParticle p)
{
    float dt = gDeltaTime;
    bool isChild = (p.flags & kFlagChild) != 0u;
    uint motion = isChild ? kMotionPhysics : gMotion;
    float3 target = (gUseLiveTarget != 0u) ? gLiveTarget : p.target;
    float3 previous = p.position;

    // このフレームを進めた後の「生まれてからの割合」(0〜1)
    float age = 1.0f - saturate((p.remainingLife - dt) / max(p.totalLife, 0.0001f));
    bool arrived = false;

    if (motion == kMotionConverge)
    {
        // 外側から、弧を描きながら目標へ吸い込まれる (寿命の終わりにちょうど到着する)
        float e = pow(age, gEasePower);
        float direction = (gArcBothDirections != 0u && p.seed > 0.5f) ? -1.0f : 1.0f;
        float3 offset = RotateAroundAxis(p.start, gAxis, gArcAngle * e * direction) * (1.0f - e);
        p.position = target + offset;
    }
    else if (motion == kMotionVortex)
    {
        // 目標を通る軸のまわりを回りながら、上昇(下降)・半径変化
        float3 offset = p.position - target;
        float height = dot(offset, gAxis);
        float3 radial = offset - gAxis * height;
        float radius = length(radial);
        radial = RotateAroundAxis(radial, gAxis, gVortexAngularSpeed * dt);
        float newRadius = max(radius + gVortexRadialSpeed * dt, 0.0f);
        if (radius > 0.0001f)
        {
            radial *= newRadius / radius;
        }
        height += gVortexRiseSpeed * dt;
        p.position = target + radial + gAxis * height;
    }
    else if (motion == kMotionHoming)
    {
        // 目標へ向かうよう、速度を少しずつ曲げる
        float3 toTarget = target - p.position;
        float dist = length(toTarget);
        float3 desired = (dist > 0.0001f) ? (toTarget / dist) * gHomingSpeed : float3(0.0f, 0.0f, 0.0f);
        p.velocity = lerp(p.velocity, desired, saturate(gHomingTurn * dt));
        if (gNoiseStrength > 0.0f)
        {
            p.velocity += FlowField(p.position * gNoiseFrequency + p.seed * 7.0f, gTime) * gNoiseStrength * dt;
        }
        p.position += p.velocity * dt;
        arrived = (length(target - p.position) < gArriveRadius);
    }
    else if (motion == kMotionBezier)
    {
        // 発生位置 → 制御点 → 目標 の2次ベジェに沿って飛ぶ
        float t = pow(age, gEasePower);
        float u = 1.0f - t;
        p.position = u * u * p.start + 2.0f * u * t * p.control + t * t * target;
    }
    else
    {
        // 自由(物理) / 放射(爆発)
        SimulatePhysics(p, dt);
    }

    // 吸い込まれて消える：消え始めたら目標へ急加速する
    if (!isChild && gDeath == kDeathAbsorb)
    {
        float k = DeathProgress(p.remainingLife, p.totalLife, gDeathRatio);
        p.position = lerp(p.position, target, saturate(gAbsorbStrength * k * k * dt));
    }

    // 位置を直接決める動き方は、見た目(伸び・明るさ)用に速度を逆算しておく
    bool physicsMotion = (motion == kMotionPhysics || motion == kMotionExplode || motion == kMotionHoming);
    if (!physicsMotion || (!isChild && gDeath == kDeathAbsorb))
    {
        p.velocity = (p.position - previous) / max(dt, 0.00001f);
    }

    p.rotation += p.rotationSpeed * dt;
    p.remainingLife = arrived ? 0.0f : (p.remainingLife - dt);

    // 寿命が尽きた瞬間に分裂
    if (p.remainingLife <= 0.0f && !isChild && gDeath == kDeathSplit && gSplitCount > 0u)
    {
        SpawnChildren(p);
    }
}

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint index = dispatchThreadId.x;
    bool alive = false;

    // NOTE: 最後に WaveActiveCountBits で生きている数を数えるので、途中で return しない
    if (index < gMaxCount)
    {
        FxParticle particle = gParticles[index];

        if (gClear != 0u)
        {
            particle.remainingLife = 0.0f;
            gParticles[index] = particle;
        }
        else if (particle.remainingLife > 0.0f)
        {
            if (gDeltaTime > 0.0f)
            {
                Simulate(particle);
                gParticles[index] = particle;
            }
            alive = (particle.remainingLife > 0.0f);
        }
        else
        {
            // 死んでいるスロット：発生の命令 → 前フレームの分裂の子 の順に引き取る
            uint claimedIndex;
            InterlockedAdd(gCounters[0], 1u, claimedIndex);

            if (claimedIndex < gSpawnCount)
            {
                // 通し番号から、どの命令の何粒目かを探す (命令は firstIndex の小さい順に並んでいる)
                uint batchIndex = 0u;
                [loop]
                for (uint b = 1u; b < gBatchCount; ++b)
                {
                    if (claimedIndex >= gBatches[b].firstIndex)
                    {
                        batchIndex = b;
                    }
                }
                SpawnBatch batch = gBatches[batchIndex];
                gParticles[index] = CreateParticle(batch, claimedIndex - batch.firstIndex);
                alive = true;
            }
            else
            {
                uint childIndex = claimedIndex - gSpawnCount;
                uint childCount = min(gCounters[1u + gChildReadSlot], gMaxCount);
                if (childIndex < childCount)
                {
                    gParticles[index] = gChildren[gChildReadSlot * gMaxCount + childIndex];
                    alive = true;
                }
            }
        }
    }

    // 生きている数を数える (1粒ずつではなく、ウェーブ単位でまとめて足す)
    uint aliveInWave = WaveActiveCountBits(alive);
    if (WaveIsFirstLane())
    {
        InterlockedAdd(gCounters[3], aliveInWave);
    }
}
