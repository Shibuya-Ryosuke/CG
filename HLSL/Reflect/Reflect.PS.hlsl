#include "Reflect.hlsli"

// 反射専用のマテリアル構造体（C++側とサイズを合わせる）
struct ReflectMaterial
{
    float4 color;
    int enableLighting;
    int shadingMode;
    float reflectionWeight; // 反射率
    float shininess;        // 光沢（将来用）
    float4x4 uvTransform;
};

// 既存のライト構造体
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// レジスタ番号は Object3d の運用に合わせる
ConstantBuffer<ReflectMaterial> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionLight : register(b1); // ※1


Texture2D<float4> gReflectTexture : register(t0); // 鏡用テクスチャ
SamplerState gSampler : register(s0);

struct PixelShaderOutput 
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) 
{
    PixelShaderOutput output;
    
    // 1. UV変換とサンプリング
    float4 transformdUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 baseColor = gReflectTexture.Sample(gSampler, transformdUV.xy);
    
    // 2. スクリーン投影による反射テクスチャのサンプリング
    float2 projectedUV = input.screenPosition.xy / input.screenPosition.w;
    projectedUV.x = projectedUV.x * 0.5f + 0.5f;
    projectedUV.y = projectedUV.y * -0.5f + 0.5f; // DirectXは上が1, 下が-1
    
    // デフォルト（0〜1）以外の場所はリピートさせない。
    float4 reflectColor = float4(1.0f,1.0f,1.0f, 1.0f); // 初期値を真っ黒（透明）に

    if (projectedUV.x >= 0.0f && projectedUV.x <= 1.0f &&
    projectedUV.y >= 0.0f && projectedUV.y <= 1.0f)
    {
    // 範囲内の時だけ、鏡テクスチャから色を持ってくる
        reflectColor = gReflectTexture.Sample(gSampler, projectedUV);
    }

    // 3. ライティング（Object3d.PS.hlsl の計算を移植）
    float4 litColor = baseColor * gMaterial.color;
    if (gMaterial.enableLighting != 0)
    {
        float cos = 0.0f;
        float3 normal = normalize(input.normal);
        float3 lightDir = -normalize(gDirectionLight.direction);

        if (gMaterial.shadingMode == 0) { // Lambert
            cos = saturate(dot(normal, lightDir));
        }
        else { // Half-Lambert
            float NdotL = dot(normal, lightDir);
            cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        }
        
        // ライティングを適用（環境光 0.3f は暫定）
        litColor.rgb *= (cos * gDirectionLight.color.rgb * gDirectionLight.intensity + 0.3f);
    }

    // 4. 反射の合成
    // baseColor(ライティング済) と reflectColor を reflectionWeight で混ぜる
    output.color.rgb = lerp(litColor.rgb, reflectColor.rgb, gMaterial.reflectionWeight);
    output.color.a = litColor.a;
    
    return output;
}