struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : TEXCOORD1;
    nointerpolation uint instanceId : TEXCOORD2; // PS側でインスタンスの色・発光などを引くため
};
