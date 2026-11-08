// カスタムポストエフェクトのピクセルシェーダー main (エンジン側の型紙)
// ShaderManager が「PostEffectCommon.hlsli → アプリのシェーダー → このファイル」の順にまとめてコンパイルする。
// 頂点シェーダーは PostProcess/Composite.VS.hlsl (画面全体を覆う三角形) をそのまま使う。

// エディタ(IntelliSense)がこのファイル単体を見ても型や関数が分かるように include しておく。
// 実際のコンパイルでは ShaderManager が先に include しているので、二重には読まれない (include guard)
#include "PostEffectCommon.hlsli"
// エディタ用: 単体で見たときだけ何もしない PostEffect を読む (実際のコンパイルでは POST_EFFECT_STAGE_PS が定義されているので読まない)
#ifndef POST_EFFECT_STAGE_PS
#include "DefaultPostEffect.hlsli"
#endif

// Composite.VS.hlsl の出力と合わせる
struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PostEffectInput effectInput;
    effectInput.uv = input.texcoord;
    effectInput.pixelPosition = input.position.xy;
    effectInput.screenSize = gScreenSize;
    effectInput.texelSize = 1.0f / max(gScreenSize, float2(1.0f, 1.0f));
    effectInput.time = gTime;
    effectInput.params = gParams;

    PixelShaderOutput output;
    output.color = PostEffect(effectInput);
    return output;
}
