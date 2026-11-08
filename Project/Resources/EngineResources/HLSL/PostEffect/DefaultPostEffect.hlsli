// 何もしない PostEffect (画面をそのまま返す)。
// アプリのシェーダーを書くときの最小のひな形 (エディタ用にも使う)。
#include "PostEffectCommon.hlsli" // エディタ(IntelliSense)用。実際のコンパイルでは先に include 済みなので二重には読まれない
float4 PostEffect(PostEffectInput input)
{
    return SampleScene(input.uv);
}
