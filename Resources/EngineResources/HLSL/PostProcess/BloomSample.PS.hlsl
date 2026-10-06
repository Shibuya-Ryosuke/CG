Texture2D<float4> gSource : register(t0);
SamplerState gSampler : register(s0);

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// ダウンサンプル・アップサンプルの両方で使う、単純に読んで出すだけのシェーダー。
// ・ダウンサンプル時：出力解像度は入力より小さいので、線形フィルタが自然にボックスフィルタ
// 　相当の平均化(=ぼかし)をしてくれる。役割はPSO側でBlendState=上書きにすることで決まる。
// ・アップサンプル時：出力解像度は入力より大きいので、線形フィルタが自然に補間拡大(にじみ)する。
// 　役割はPSO側でBlendState=加算にすることで、既存の内容に足し込む形になる。
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    output.color = gSource.Sample(gSampler, input.texcoord);
    return output;
}
