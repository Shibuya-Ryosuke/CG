// シャドウマップ生成用の頂点シェーダー。
// 深度だけ書ければいいので、出力はSV_Positionのみ(ピクセルシェーダーは無し)。
//
// NOTE: TransformationMatrix構造体はModel_VS.hlslと同じレイアウトを保っている
//       (C++側のroot param 1のCBVを、そのまま同じ構造として読むだけのため)。
//       WVP(カメラ視点)はここでは使わず、Worldだけを使ってライト視点のLightViewProjを掛ける。

struct TransformationMatrix
{
    float4x4 WVP;      // このシェーダーでは未使用
    float4x4 World;
    float alpha;       // このシェーダーでは未使用
    float3 padding;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// ライトのView-Projection行列 (C++側のroot param 6。シャドウパスでは頂点シェーダーのみが使う)
cbuffer LightViewProjBuffer : register(b3)
{
    float4x4 gLightViewProj;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
};

VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float4 worldPosition = mul(input.position, gTransformationMatrix.World);
    output.position = mul(worldPosition, gLightViewProj);
    return output;
}
