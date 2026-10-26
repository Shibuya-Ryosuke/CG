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

// ブルームのアップサンプル(拡大)用シェーダー。
// 1回のバイリニアで低解像度の段を引き伸ばすと、元の1テクセルがブロック状のムラとして見えるため、
// 3x3のテントフィルタ(9点、重み 1-2-1 / 2-4-2 / 1-2-1 の合計16)でなめらかに拡大する。
// 加算合成(既存の内容への足し込み)は、これまで通りPSO側のBlendStateで行う。
// テクセルの大きさは入力テクスチャのサイズから求めるので、C++側から渡す必要はない。
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float width, height;
    gSource.GetDimensions(width, height);
    float2 texel = float2(1.0f / width, 1.0f / height);
    float2 uv = input.texcoord;

    float3 color = gSource.Sample(gSampler, uv).rgb * 4.0f;

    color += gSource.Sample(gSampler, uv + texel * float2( 0.0f,  1.0f)).rgb * 2.0f;
    color += gSource.Sample(gSampler, uv + texel * float2( 0.0f, -1.0f)).rgb * 2.0f;
    color += gSource.Sample(gSampler, uv + texel * float2( 1.0f,  0.0f)).rgb * 2.0f;
    color += gSource.Sample(gSampler, uv + texel * float2(-1.0f,  0.0f)).rgb * 2.0f;

    color += gSource.Sample(gSampler, uv + texel * float2(-1.0f,  1.0f)).rgb;
    color += gSource.Sample(gSampler, uv + texel * float2( 1.0f,  1.0f)).rgb;
    color += gSource.Sample(gSampler, uv + texel * float2(-1.0f, -1.0f)).rgb;
    color += gSource.Sample(gSampler, uv + texel * float2( 1.0f, -1.0f)).rgb;

    output.color = float4(color / 16.0f, 1.0f);
    return output;
}
