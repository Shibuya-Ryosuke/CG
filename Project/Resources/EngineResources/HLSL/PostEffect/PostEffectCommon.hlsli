#ifndef POST_EFFECT_COMMON_HLSLI
#define POST_EFFECT_COMMON_HLSLI

// =====================================================================
// カスタムポストエフェクトの共通部分 (エンジン側。アプリのシェーダーは触らなくてよい)
//
// アプリのシェーダーファイルに書くもの:
//   ・PostEffect (必須) : 1ピクセルの色を返す関数
//       float4 PostEffect(PostEffectInput input) { ... }
//       SampleScene(uv) で「1つ前までの画面」の色が読めるので、それを加工して返す。
//       何もしない場合は return SampleScene(input.uv);
//
// main はエンジン側 (PostEffectMain.PS.hlsli) が持っているので、アプリは書かない。
// かかる場所: トーンマッピングの後 (色は0〜1)、UI(2D)を描く前。
// 複数の効果は PostEffectManager::Add した順に重ねてかかる。
//
// ShaderManager がコンパイル時に、次の順番で1つのファイルにまとめる:
//   #define POST_EFFECT_STAGE_PS
//   #include "PostEffect/PostEffectCommon.hlsli"   ← このファイル
//   #include "アプリのシェーダー"                   ← PostEffect()
//   #include "PostEffect/PostEffectMain.PS.hlsli"  ← main
// =====================================================================

// 1つ前までの画面 (C++側 root param 0)
Texture2D<float4> gSceneTexture : register(t0);
// シェーダーごとの追加テクスチャ (ShaderManager::SetTexture の 0番 / 1番。C++側 root param 1 / 2)
// 設定していない場合は white1x1 が入っている
Texture2D<float4> gShaderTexture0 : register(t1);
Texture2D<float4> gShaderTexture1 : register(t2);
// 画面用 (端で折り返さない) / 追加テクスチャ用 (タイル状に繰り返す)
SamplerState gClampSampler : register(s0);
SamplerState gWrapSampler : register(s1);

// C++側 root param 3 のルート定数 (PostEffectManager::DrawEffect と合わせる)
cbuffer PostEffectConstants : register(b0)
{
    float4 gParams[2];      // 自由な値 (PostEffectManager::SetParam。0〜3番 = gParams[0].xyzw、4〜7番 = gParams[1].xyzw)
    float2 gScreenSize;     // 画面の解像度 (ピクセル)
    float gTime;            // 経過時間(秒)。TimeManagerの時間の速さ(スロー等)に合わせて進む
    float gPadding;
};

// --- PostEffect() に渡される値 ---
struct PostEffectInput
{
    float2 uv;              // 画面上の位置 (左上(0,0) 〜 右下(1,1))
    float2 pixelPosition;   // 画面上の位置 (ピクセル)
    float2 screenSize;      // 画面の解像度 (ピクセル)
    float2 texelSize;       // 1ピクセル分のUV (隣のピクセルを読むとき用)
    float time;             // 経過時間(秒)
    float4 params[2];       // 自由な値 (0〜3番 = params[0].xyzw、4〜7番 = params[1].xyzw)
};

// --- 便利関数 ---
// 1つ前までの画面の色 (uv が 0〜1 の外なら端の色)
float4 SampleScene(float2 uv) { return gSceneTexture.Sample(gClampSampler, uv); }
// 追加テクスチャ (uv が 0〜1 の外ならタイル状に繰り返す)
float4 SampleShaderTexture0(float2 uv) { return gShaderTexture0.Sample(gWrapSampler, uv); }
float4 SampleShaderTexture1(float2 uv) { return gShaderTexture1.Sample(gWrapSampler, uv); }
// 明るさ (白黒にするときなど)
float Luminance(float3 color) { return dot(color, float3(0.2126f, 0.7152f, 0.0722f)); }

#endif // POST_EFFECT_COMMON_HLSLI
