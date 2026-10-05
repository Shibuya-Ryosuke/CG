// 毎フレーム、シミュレーション本体パスの直前に1スレッドだけ実行する。
// 本体パスは「死んでいるスロットが、このカウンターをInterlockedAddで進めながら
// 発生依頼リストの何番目を担当するか決める」という使い方をするため、フレームの頭で0に戻しておく必要がある。
// (同じDispatch内の別グループ間には実行順序の保証も同期も無いので、本体パスの中で
//  スレッド0が0に戻す、という方法は使えない。別パスに分けている理由はこれ)

RWStructuredBuffer<uint> gClaimCounter : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    gClaimCounter[0] = 0;
}
