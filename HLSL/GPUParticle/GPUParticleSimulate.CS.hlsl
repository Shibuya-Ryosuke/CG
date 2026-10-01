// C++側 GPUParticleData構造体(GPUParticleCommon.h)と1:1でレイアウトを合わせること。全体で64byte。
struct ParticleData
{
    float3 position;
    float  scale;
    float3 velocity;
    float  totalLife;
    float4 color;
    float3 rotation;       // オイラー角(ラジアン)。ビルボード時はzのみ使う
    float  remainingLife;
};

RWStructuredBuffer<ParticleData> gParticles : register(u0);      // パーティクル本体(読み書き)
RWStructuredBuffer<uint> gClaimCounter : register(u1);            // 発生依頼を何番目まで消化したか(Atomic)
StructuredBuffer<ParticleData> gSpawnRequests : register(t0);     // 今フレームの発生依頼(読み取り専用)

cbuffer SimParams : register(b0)
{
    float    gDeltaTime;
    uint     gSpawnRequestCount; // 今フレームの発生依頼数
    uint     gMaxParticleCount;  // このEmitterのスロット総数
    float    gGravity;           // 下向き(-Y)の加速度。0なら無重力
};

[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint index = dispatchThreadId.x;
    if (index >= gMaxParticleCount)
    {
        return; // スロット数が256の倍数でない場合、はみ出したスレッドは何もしない
    }

    ParticleData particle = gParticles[index];

    if (particle.remainingLife > 0.0f)
    {
        // --- 生きているスロット：物理演算を1フレーム分進める ---
        particle.velocity.y -= gGravity * gDeltaTime;
        particle.position += particle.velocity * gDeltaTime;
        particle.remainingLife -= gDeltaTime;
        gParticles[index] = particle;
    }
    else
    {
        // --- 死んでいるスロット：発生依頼があれば自分が引き取れるか試す ---
        // カウンターを原子的に1つ進め、「自分がその瞬間何番目の希望者だったか」を取得する。
        // これにより、同時に多数のスロットが死んでいても、発生依頼の数だけ重複無く配られる。
        uint claimedIndex;
        InterlockedAdd(gClaimCounter[0], 1, claimedIndex);

        if (claimedIndex < gSpawnRequestCount)
        {
            // 依頼の内容(初期位置・速度・色等。remainingLife=totalLifeの状態)でそのまま復活させる
            gParticles[index] = gSpawnRequests[claimedIndex];
        }
        // 依頼を取り尽くしていた場合は、何も書き戻さず死んだままにしておく
    }
}
