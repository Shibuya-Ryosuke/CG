// モデルのシャドウマップ描画の頂点シェーダー main (エンジン側の型紙)
// ShaderManager が「ModelSurface.hlsli → アプリのシェーダー → このファイル」の順にまとめてコンパイルする。
// アプリの Vertex() で頂点を動かしている場合、影も同じように動く。
// (旧 ModelBatchShadow.VS.hlsl の処理に、アプリの Vertex() を呼ぶ処理を足したもの)
// エディタ(IntelliSense)がこのファイル単体を見ても型や関数が分かるように include しておく。
// 実際のコンパイルでは ShaderManager が先に include しているので、二重には読まれない (include guard)
#include "ModelSurface.hlsli"

// ライトのView-Projection (C++側 root param 6)
cbuffer LightViewProjBuffer : register(b3)
{
    float4x4 gLightViewProj;
};

struct ShadowVertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct ShadowVertexShaderOutput
{
    float4 position : SV_Position;
};

ShadowVertexShaderOutput main(ShadowVertexShaderInput input, uint instanceId : SV_InstanceID)
{
    ShadowVertexShaderOutput output;
    ModelInstance instance = gInstances[instanceId];

    VertexInput v;
    v.position = input.position;
    v.texcoord = input.texcoord;
    v.normal = input.normal;
#ifdef USE_CUSTOM_VERTEX
    Vertex(v, instance);
#endif

    float4 worldPosition = mul(v.position, instance.world);
    output.position = mul(worldPosition, gLightViewProj);
    return output;
}
