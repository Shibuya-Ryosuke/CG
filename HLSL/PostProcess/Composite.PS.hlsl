Texture2D<float4> gSceneColor : register(t0);
Texture2D<float4> gBloomTexture : register(t1); // ブルームOFF時は未使用(有効な値は入っているが参照しない)
SamplerState gSampler : register(s0);

cbuffer PostProcessParams : register(b0)
{
    uint gACESEnabled;   // ON:ACESフィルミックで圧縮 / OFF:露出後の値を単純にクリップ
    uint gBloomEnabled;  // ON:ブルーム結果をシーンカラーに加算してから圧縮する
    float gExposure;     // 圧縮する前に、明るさを底上げ/引き下げする係数(常時有効)
    float gThreshold;    // このパスでは未使用(BloomThresholdパス用)
    float gBloomIntensity; // ブルームをシーンへ加算する際の強さ
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

// ACESフィルミックトーンマッピングの近似式 (Narkowicz 2015)
// HDRの値(0〜∞)を、なめらかに0〜1へ圧縮する。Reinhard(hdrColor/(hdrColor+1))と違い、
// 中間調のコントラストを保ちながら明るい部分だけをなだらかに丸めるよう設計されている。
float3 ACESFilm(float3 x)
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float3 hdrColor = gSceneColor.Sample(gSampler, input.texcoord).rgb;

    // ブルーム：閾値を超えた明るい部分がダウンサンプル/アップサンプルを経て滲んだ結果を加算する
    if (gBloomEnabled != 0)
    {
        float3 bloomColor = gBloomTexture.Sample(gSampler, input.texcoord).rgb;
        hdrColor += bloomColor * gBloomIntensity;
    }

    // 露出：圧縮する前に、シーン全体の明るさを底上げ/引き下げする(常時適用)
    float3 exposedColor = hdrColor * gExposure;

    float3 finalColor;
    if (gACESEnabled != 0)
    {
        finalColor = ACESFilm(exposedColor);
    }
    else
    {
        // ACES OFF：露出後の値を単純にクリップするだけ
        finalColor = saturate(exposedColor);
    }

    output.color = float4(finalColor, 1.0f);
    return output;
}
