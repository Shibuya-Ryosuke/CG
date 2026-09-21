Texture2D<float4> gSource : register(t0);
SamplerState gSampler : register(s0);

cbuffer PostProcessParams : register(b0)
{
    uint gACESEnabled;    // このパスでは未使用
    uint gBloomEnabled;   // このパスでは未使用
    float gExposure;      // このパスでは未使用
    float gThreshold;     // これを超えた明るさの部分だけを取り出す
    float gBloomIntensity; // このパスでは未使用
    float3 padding;
};

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
    PixelShaderOutput output;

    // NOTE: 出力先(bloomLevels_[0])は入力(シーンカラー)の半分の解像度。
    //       線形フィルタでサンプリングすることで、1回のサンプルで自然に2x2の
    //       ボックスフィルタ相当の平均化(=簡易ダウンサンプル)も同時に行っている。
    float3 color = gSource.Sample(gSampler, input.texcoord).rgb;

    // 閾値を超えた分だけを取り出すハードニー。超えていなければ真っ黒(0)になる
    float3 brightPart = max(color - gThreshold, 0.0f);

    output.color = float4(brightPart, 1.0f);
    return output;
}
