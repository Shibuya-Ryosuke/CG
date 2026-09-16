struct VertexShaderOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
    float3 worldPosition : TEXCOORD1; // Point/Spot/AreaLightの計算に使うワールド座標
};
