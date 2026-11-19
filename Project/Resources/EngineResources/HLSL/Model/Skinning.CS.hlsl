// スキニング (コンピュートシェーダー)
// 頂点1つにつき1スレッド。ボーンの行列パレットで頂点と法線を動かし、普通のモデルと同じ形の頂点として書き出す。
// 書き出した頂点は、そのまま頂点バッファとして ModelMain.VS.hlsli で描かれる (影・カスタムシェーダーもそのまま使える)。
// C++側: SkinningManager.cpp (root 0: t0 / 1: t1 / 2: u0 / 3: b0)

// 行列パレットの1要素 (C++側 SkinningManager::Well と合わせる)
struct Well
{
    float4x4 skeletonSpaceMatrix;                   // 頂点を動かす行列
    float4x4 skeletonSpaceInverseTransposeMatrix;   // 法線を動かす行列 (逆転置)
};

// 入力の頂点 (C++側 SkinVertexData と合わせる。68byte)
struct SkinVertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
    float4 weight;  // ボーンの割合 (合計1)
    uint4 index;    // 行列パレットの番号
};

// 出力の頂点 (C++側 VertexData と合わせる。36byte。頂点バッファとして読まれる)
struct Vertex
{
    float4 position;
    float2 texcoord;
    float3 normal;
};

StructuredBuffer<Well> gMatrixPalette : register(t0);
StructuredBuffer<SkinVertex> gInputVertices : register(t1);
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);

cbuffer SkinningInformation : register(b0)
{
    uint gNumVertices;
};

// C++側 SkinningManager::kThreadGroupSize と合わせる
[numthreads(256, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    uint vertexIndex = dispatchThreadId.x;
    if (vertexIndex >= gNumVertices)
    {
        return;
    }

    SkinVertex input = gInputVertices[vertexIndex];
    Vertex output;
    output.texcoord = input.texcoord;

    float totalWeight = input.weight.x + input.weight.y + input.weight.z + input.weight.w;
    if (totalWeight <= 0.0f)
    {
        // ボーンのウェイトが無い頂点は、そのまま
        output.position = input.position;
        output.normal = input.normal;
    }
    else
    {
        float3 position = float3(0.0f, 0.0f, 0.0f);
        float3 normal = float3(0.0f, 0.0f, 0.0f);
        [unroll]
        for (uint i = 0; i < 4; ++i)
        {
            float weight = input.weight[i];
            if (weight > 0.0f)
            {
                Well well = gMatrixPalette[input.index[i]];
                position += mul(input.position, well.skeletonSpaceMatrix).xyz * weight;
                normal += mul(input.normal, (float3x3) well.skeletonSpaceInverseTransposeMatrix) * weight;
            }
        }
        output.position = float4(position, 1.0f);
        float normalLength = length(normal);
        output.normal = normalLength > 0.0f ? normal / normalLength : input.normal;
    }

    gOutputVertices[vertexIndex] = output;
}
