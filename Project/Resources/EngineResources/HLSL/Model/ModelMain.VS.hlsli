// モデル描画の頂点シェーダー main (エンジン側の型紙)
// ShaderManager が「ModelSurface.hlsli → アプリのシェーダー → このファイル」の順にまとめてコンパイルする。
// (旧 ModelBatch.VS.hlsl の処理に、アプリの Vertex() を呼ぶ処理を足したもの)
// エディタ(IntelliSense)がこのファイル単体を見ても型や関数が分かるように include しておく。
// 実際のコンパイルでは ShaderManager が先に include しているので、二重には読まれない (include guard)
#include "ModelSurface.hlsli"
#include "ModelBatch.hlsli"

// カメラのView-Projection (C++側 root param 0) は ModelSurface.hlsli の CameraBuffer (PSでもカメラ位置を使うため移動)

// 法線をワールド空間へ変換する。
// 拡縮が軸ごとに違う(非均一スケール)と、ワールド行列をそのまま掛けた法線は面に垂直でなくなるので、
// 逆転置行列 (M^-1)^T を掛ける必要がある。(M^-1)^T = 余因子行列 / 行列式 なので、
// 逆行列を計算せずに余因子行列(各行 = 他の2行の外積)を掛け、行列式の符号だけ合わせて正規化する。
// (正規化するので行列式の大きさは不要。負の拡縮で裏返った場合だけ向きを戻す)
float3 TransformNormal(float3 normal, float4x4 world)
{
    float3x3 m = (float3x3) world;
    float3x3 cofactor = float3x3(
        cross(m[1], m[2]),
        cross(m[2], m[0]),
        cross(m[0], m[1]));
    float det = dot(m[0], cofactor[0]);
    float3 n = mul(normal, cofactor) * (det < 0.0f ? -1.0f : 1.0f);
    return normalize(n);
}

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
    output.normal = TransformNormal(v.normal, instance.world);

    // UV変換: uv' = uv * [m00 m01; m10 m11] + offset  (旧 mul(float4(uv,0,1), uvTransform) と同じ)
    float2 uv = v.texcoord;
    output.texcoord = float2(
        uv.x * instance.uvMat.x + uv.y * instance.uvMat.z,
        uv.x * instance.uvMat.y + uv.y * instance.uvMat.w) + instance.uvOffset;

    output.instanceId = instanceId;
    return output;
}
