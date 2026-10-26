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

// ブルームのダウンサンプル(縮小)用シェーダー。
// 1回のバイリニアだけだと縮小を重ねたときにチラつきやムラが出やすいので、
// 入力テクスチャの周囲13点を重み付きで平均する(Call of Duty: Advanced Warfare方式)。
//
//   a . b . c
//   . j . k .
//   d . e . f
//   . l . m .
//   g . h . i
//
// 中央の4点(j,k,l,m)の箱に0.5、四隅の4つの箱(a,b,d,e / b,c,e,f / d,e,g,h / e,f,h,i)に0.125ずつの重みを付ける。
// テクセルの大きさは入力テクスチャのサイズから求めるので、C++側から渡す必要はない。
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float width, height;
    gSource.GetDimensions(width, height);
    float2 texel = float2(1.0f / width, 1.0f / height);
    float2 uv = input.texcoord;

    float3 a = gSource.Sample(gSampler, uv + texel * float2(-2.0f,  2.0f)).rgb;
    float3 b = gSource.Sample(gSampler, uv + texel * float2( 0.0f,  2.0f)).rgb;
    float3 c = gSource.Sample(gSampler, uv + texel * float2( 2.0f,  2.0f)).rgb;

    float3 d = gSource.Sample(gSampler, uv + texel * float2(-2.0f,  0.0f)).rgb;
    float3 e = gSource.Sample(gSampler, uv).rgb;
    float3 f = gSource.Sample(gSampler, uv + texel * float2( 2.0f,  0.0f)).rgb;

    float3 g = gSource.Sample(gSampler, uv + texel * float2(-2.0f, -2.0f)).rgb;
    float3 h = gSource.Sample(gSampler, uv + texel * float2( 0.0f, -2.0f)).rgb;
    float3 i = gSource.Sample(gSampler, uv + texel * float2( 2.0f, -2.0f)).rgb;

    float3 j = gSource.Sample(gSampler, uv + texel * float2(-1.0f,  1.0f)).rgb;
    float3 k = gSource.Sample(gSampler, uv + texel * float2( 1.0f,  1.0f)).rgb;
    float3 l = gSource.Sample(gSampler, uv + texel * float2(-1.0f, -1.0f)).rgb;
    float3 m = gSource.Sample(gSampler, uv + texel * float2( 1.0f, -1.0f)).rgb;

    float3 color = e * 0.125f;
    color += (a + c + g + i) * 0.03125f;
    color += (b + d + f + h) * 0.0625f;
    color += (j + k + l + m) * 0.125f;

    output.color = float4(color, 1.0f);
    return output;
}
