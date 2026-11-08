// モデル描画の頂点シェーダー main (エンジン側の型紙)
// ShaderManager が「ModelSurface.hlsli → アプリのシェーダー → このファイル」の順にまとめてコンパイルする。
// (旧 ModelBatch.VS.hlsl の処理に、アプリの Vertex() を呼ぶ処理を足したもの)
// エディタ(IntelliSense)がこのファイル単体を見ても型や関数が分かるように include しておく。
// 実際のコンパイルでは ShaderManager が先に include しているので、二重には読まれない (include guard)
#include "ModelSurface.hlsli"
#include "ModelBatch.hlsli"

// カメラのView-Projection (C++側 root param 0)
cbuffer CameraBuffer : register(b0)
{
    float4x4 gViewProjection;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID)
{
    VertexShaderOutput output;
    ModelInstance instance = gInstances[instanceId];

    VertexInput v;
    v.position = input.position;
    v.texcoord = input.texcoord;
    v.normal = input.normal;
#ifdef USE_CUSTOM_VERTEX
    // アプリのシェーダーの頂点変形 (ローカル座標)
    Vertex(v, instance);
#endif

    float4 worldPosition = mul(v.position, instance.world);
    output.position = mul(worldPosition, gViewProjection);
    output.worldPosition = worldPosition.xyz;
    output.normal = normalize(mul(v.normal, (float3x3) instance.world));

    // UV変換: uv' = uv * [m00 m01; m10 m11] + offset  (旧 mul(float4(uv,0,1), uvTransform) と同じ)
    float2 uv = v.texcoord;
    output.texcoord = float2(
        uv.x * instance.uvMat.x + uv.y * instance.uvMat.z,
        uv.x * instance.uvMat.y + uv.y * instance.uvMat.w) + instance.uvOffset;

    output.instanceId = instanceId;
    return output;
}
