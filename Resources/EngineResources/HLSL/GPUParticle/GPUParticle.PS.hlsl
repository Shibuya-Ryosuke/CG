Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0; // VS側で寿命フェード込み(rgb=パーティクル色, a=フェード適用済みの不透明度)
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// NOTE: ライティング計算(法線・Light配列・シャドウ等)は意図的に一切行わない。
//       テクスチャの色にパーティクル自身の色(寿命フェード込み)を掛けるだけ。
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor * input.color;
    return output;
}
