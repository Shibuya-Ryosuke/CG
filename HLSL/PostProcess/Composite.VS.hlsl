// 頂点バッファを使わず、SV_VertexID(0,1,2)だけから画面全体を覆う大きな三角形を生成する定番のテクニック。
// 頂点0:(-1,-1) 頂点1:(-1,3) 頂点2:(3,-1) のような、画面より大きい三角形を1枚描くことで
// 四角形(2枚のポリゴン)を使うより少ない頂点数で画面全体をカバーできる。

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

VertexShaderOutput main(uint vertexId : SV_VertexID)
{
    VertexShaderOutput output;

    // vertexId: 0,1,2 → texcoord: (0,0),(2,0),(0,2) となるように計算
    output.texcoord = float2((vertexId << 1) & 2, vertexId & 2);

    // texcoord(0〜2)を、NDC座標(-1〜1、Yは上向き)に変換
    output.position = float4(output.texcoord.x * 2.0f - 1.0f, 1.0f - output.texcoord.y * 2.0f, 0.0f, 1.0f);

    return output;
}
