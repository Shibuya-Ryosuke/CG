// 毎フレーム、シミュレーション本体パスの直前に1スレッドだけ実行する。
// [0] 発生依頼をどこまで配ったか      → 0に戻す
// [1],[2] 分裂の子のリスト(2枚)の件数 → 今フレーム書き込む側だけ0に戻す (読む側は前フレームの結果)
// [3] 生きている粒の数              → 0に戻す (本体パスで数え直す。エディタの表示用)
#define PARTICLE_FX_SIMULATION
#include "Particle.hlsli"

RWStructuredBuffer<uint> gCounters : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    gCounters[0] = 0;
    gCounters[3] = 0;

    uint writeSlot = 1u - gChildReadSlot;
    gCounters[1u + writeSlot] = 0;

    if (gClear != 0u)
    {
        gCounters[1] = 0;
        gCounters[2] = 0;
    }
}
