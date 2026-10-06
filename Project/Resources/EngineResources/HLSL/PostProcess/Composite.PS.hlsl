Texture2D<float4> gSceneColor : register(t0);
Texture2D<float4> gBloomTexture : register(t1); // ブルームOFF時は未使用(有効な値は入っているが参照しない)
SamplerState gSampler : register(s0);

// NOTE: C++側 PostProcess::PostProcessParams と1:1でレイアウト・順序を合わせること
cbuffer PostProcessParams : register(b0)
{
    uint gACESEnabled; // ON:ACESフィルミックで圧縮 / OFF:露出後の値を単純にクリップ
    uint gBloomEnabled; // ON:ブルーム結果をシーンカラーに加算してから圧縮する
    float gExposure; // 圧縮する前に、明るさを底上げ/引き下げする係数(常時有効)
    float gThreshold; // このパスでは未使用(BloomThresholdパス用)
    float gBloomIntensity; // ブルームをシーンへ加算する際の強さ

    uint gDistortionEnabled;
    float gDistortionStrength;

    uint gGlitchEnabled;
    float gGlitchIntensity;

    uint gChromaticAberrationEnabled;
    float gChromaticAberrationStrength;

    uint gBlurEnabled;
    float gBlurStrength;

    uint gGrayscaleEnabled;
    float gGrayscaleIntensity;

    uint gNoiseEnabled;
    float gNoiseIntensity;

    float gTime;
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

// 疑似乱数生成(ノイズ/グリッチで共通に使う、定番の1行乱数)
float Random(float2 seed)
{
    return frac(sin(dot(seed, float2(12.9898f, 78.233f))) * 43758.5453f);
}

// 画面中心からの歪み(陽炎/画面揺らぎのような効果)。UVをサンプリングする前に適用する
float2 ApplyDistortion(float2 uv, float time, float strength)
{
    float2 centered = uv - 0.5f;
    float dist = length(centered);
    // 中心からの距離に応じたサイン波でUVを揺らす
    float wave = sin(dist * 20.0f - time * 3.0f) * strength;
    float2 dir = dist > 0.0001f ? centered / dist : float2(0.0f, 0.0f);
    return uv + dir * wave;
}

// 横一列(ブロック)ごとに、一定間隔でランダムにUVをずらすデジタルグリッチ
float2 ApplyGlitch(float2 uv, float time, float intensity)
{
    float blockY = floor(uv.y * 40.0f); // 画面を40行のブロックに分割
    float blockSeed = floor(time * 8.0f); // 1秒間に8回、乱数の種を切り替える(パラパラ動く感じにする)

    // このブロック・この瞬間だけの乱数
    float shift = (Random(float2(blockY, blockSeed)) - 0.5f) * intensity;

    // 常に全ブロックが動くとうるさいので、一定確率でしか発動させない
    float trigger = step(0.85f, Random(float2(blockSeed, 0.123f)));

    uv.x += shift * trigger;
    return uv;
}

// ブラー適用込みでシーンカラーをサンプリングする(色収差からも呼ぶことで、両方同時に効かせられる)
float3 SampleScene(float2 uv)
{
    if (gBlurEnabled != 0)
    {
        // NOTE: 簡易版の3x3ボックスブラー。gBlurStrengthはUV空間での直接のオフセット量として扱っている
        //       (本来は画面解像度に応じたテクセルサイズを使うべきだが、簡易実装のため固定的に扱う)
        float2 offset = float2(gBlurStrength, gBlurStrength);
        float3 sum = float3(0.0f, 0.0f, 0.0f);
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            [unroll]
            for (int y = -1; y <= 1; ++y)
            {
                sum += gSceneColor.Sample(gSampler, uv + float2(x, y) * offset).rgb;
            }
        }
        return sum / 9.0f;
    }
    return gSceneColor.Sample(gSampler, uv).rgb;
}

// 色収差(Chromatic Aberration)：RGB各チャンネルを少しずつ違うUVでサンプリングする
float3 SampleSceneWithChromaticAberration(float2 uv, float strength)
{
    float2 centered = uv - 0.5f;
    float2 dir = centered * strength;
    float r = SampleScene(uv - dir).r;
    float g = SampleScene(uv).g;
    float b = SampleScene(uv + dir).b;
    return float3(r, g, b);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    // --- UVを歪ませる系のエフェクト(実際に色をサンプリングする前に適用する) ---
    if (gDistortionEnabled != 0)
    {
        uv = ApplyDistortion(uv, gTime, gDistortionStrength);
    }
    if (gGlitchEnabled != 0)
    {
        uv = ApplyGlitch(uv, gTime, gGlitchIntensity);
    }

    // --- サンプリング方法そのものを変えるエフェクト(ブラー/色収差) ---
    float3 hdrColor;
    if (gChromaticAberrationEnabled != 0)
    {
        // 色収差は内部でSampleScene()を3回呼ぶため、ブラーが有効なら自動的に「ぼけた色収差」になる
        hdrColor = SampleSceneWithChromaticAberration(uv, gChromaticAberrationStrength);
    }
    else
    {
        hdrColor = SampleScene(uv);
    }

    // ブルーム：閾値を超えた明るい部分がダウンサンプル/アップサンプルを経て滲んだ結果を加算する
    if (gBloomEnabled != 0)
    {
        float3 bloomColor = gBloomTexture.Sample(gSampler, uv).rgb;
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

    // --- ここから先は、露出・トーンマッピングを終えた最終画像に対する仕上げ処理 ---

    // 白黒化(Grayscale)：輝度値へ変換し、元の色とブレンドする
    if (gGrayscaleEnabled != 0)
    {
        float luminance = dot(finalColor, float3(0.299f, 0.587f, 0.114f));
        finalColor = lerp(finalColor, float3(luminance, luminance, luminance), gGrayscaleIntensity);
    }

    // ノイズ(グレイン)：フィルム写真のようなザラつきを最後に足す
    if (gNoiseEnabled != 0)
    {
        float noise = Random(uv * 1000.0f + gTime) - 0.5f;
        finalColor += noise * gNoiseIntensity;
    }

    output.color = float4(saturate(finalColor), 1.0f);
    return output;
}
