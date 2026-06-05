struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : POSITION0; // 反射の計算（視線ベクトル等）に使う場合
    float4 screenPosition : TEXCOORD1; // スクリーン空間での反射サンプリング用
};