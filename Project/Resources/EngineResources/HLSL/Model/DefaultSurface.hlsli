// エンジン標準の Surface (何も変えない = いつもの見た目)
// カスタムシェーダーを指定していないモデルは、これで描かれる。
// アプリのシェーダーを書くときの最小のひな形にもなる。
#include "ModelSurface.hlsli" // エディタ(IntelliSense)用。実際のコンパイルでは先に include 済みなので二重には読まれない
void Surface(SurfaceInput input, inout SurfaceOutput output)
{
}
