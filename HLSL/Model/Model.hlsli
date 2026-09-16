struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float alpha : ALPHA0;
    float3 worldPosition : TEXCOORD1; // Point/Spot/AreaLightの計算に使うワールド座標
};