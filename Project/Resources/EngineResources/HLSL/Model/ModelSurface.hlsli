#ifndef MODEL_SURFACE_HLSLI
#define MODEL_SURFACE_HLSLI

// =====================================================================
// モデル用カスタムシェーダーの共通部分 (エンジン側。アプリのシェーダーは触らなくてよい)
//
// アプリのシェーダーファイルに書くもの:
//   ・Surface (必須) : ピクセルの色を決める関数
//       void Surface(SurfaceInput input, inout SurfaceOutput output) { ... }
//       output には最初から「いつもの見た目」(モデルの色 × テクスチャ、発光) が入っているので、
//       変えたいところだけ書き換えればよい。何もしなければエンジン標準と同じ見た目になる
//   ・Vertex (任意) : 頂点を動かす関数。使うときは関数の前に #define USE_CUSTOM_VERTEX を書く
//       #define USE_CUSTOM_VERTEX
//       void Vertex(inout VertexInput v, ModelInstance instance) { ... }
//       v はモデルのローカル座標。影にも同じ変形がかかる
//
// main はエンジン側 (ModelMain.*.hlsli) が持っているので、アプリは書かない。
// カメラに近いときに消す処理・ライティング・影は、エンジン側で必ず行われる。
//
// ShaderManager がコンパイル時に、次の順番で1つのファイルにまとめる:
//   #define MODEL_STAGE_VS / MODEL_STAGE_PS / MODEL_STAGE_SHADOW (どれか1つ)
//   #include "Model/ModelSurface.hlsli"   ← このファイル
//   #include "アプリのシェーダー"          ← Surface / Vertex
//   #include "Model/ModelMain.??.hlsli"   ← main
// =====================================================================

#include "ModelInstance.hlsli"

// --- テクスチャ ---
// メインテクスチャ (モデルごと。SetTex / mtl の map_Kd)。C++側 root param 2
Texture2D<float4> gTexture : register(t0);
// シェーダーごとの追加テクスチャ (ShaderManager::SetTexture の 0番 / 1番)。C++側 root param 9 / 10
// 設定していない場合は white1x1 が入っている
Texture2D<float4> gShaderTexture0 : register(t4);
Texture2D<float4> gShaderTexture1 : register(t5);
SamplerState gSampler : register(s0);

// --- 描画パスごとの値 (C++側 root param 8 のルート定数。ModelManager::SetDrawParams と合わせる) ---
cbuffer DrawConstants : register(b4)
{
    uint gBlendMode;        // C++側 BlendMode と同じ番号
    float gNearFadeStart;   // この距離以上では完全に見える
    float gNearFadeEnd;     // この距離以下では完全に消える
    float gTime;            // 経過時間(秒)。TimeManagerの時間の速さ(スロー等)に合わせて進む
};

static const uint BLEND_NONE     = 0;
static const uint BLEND_NORMAL   = 1;
static const uint BLEND_ADD      = 2;
static const uint BLEND_SUBTRACT = 3;
static const uint BLEND_MULTIPLY = 4;
static const uint BLEND_SCREEN   = 5;

// --- Vertex() に渡される頂点 (モデルのローカル座標) ---
struct VertexInput
{
    float4 position;
    float2 texcoord;    // UV変換(SetUVSRT)の前の値
    float3 normal;
};

// --- Surface() に渡される値 ---
struct SurfaceInput
{
    float2 uv;              // UV変換(SetUVSRT)の後の値
    float3 normal;          // ワールド空間の法線 (正規化済み)
    float3 worldPosition;   // ワールド座標
    float4 screenPosition;  // SV_Position (xy = 画面のピクセル座標)
    float4 params[2];       // 自由な値 (Model::SetShaderParam。0〜3番 = params[0].xyzw、4〜7番 = params[1].xyzw)
    float time;             // 経過時間(秒)
    ModelInstance instance; // モデルの色・発光など (instance.color など)
};

// --- Surface() で書き換える値 (最初から「いつもの見た目」が入っている) ---
struct SurfaceOutput
{
    float3 color;           // 色 (初期値: モデルの色 × メインテクスチャ)
    float alpha;            // アルファ (初期値: モデルの色のa × メインテクスチャのa)。Normal/Add/Subtract で使われる
    float3 emissive;        // 発光 (初期値: SetEmissive の色 × 強さ)。ライティングを通さず最後に足される
    float3 normal;          // ライティングに使う法線 (初期値: input.normal)
    bool enableLighting;    // ライティングするか (初期値: SetEnableLighting / SetLambert の設定)
};

// --- 便利関数 ---
// 頂点シェーダーではミップマップを選べないので、0番のミップを読む
float4 SampleTexture(Texture2D<float4> tex, float2 uv)
{
#ifdef MODEL_STAGE_PS
    return tex.Sample(gSampler, uv);
#else
    return tex.SampleLevel(gSampler, uv, 0.0f);
#endif
}
float4 SampleMainTexture(float2 uv) { return SampleTexture(gTexture, uv); }
float4 SampleShaderTexture0(float2 uv) { return SampleTexture(gShaderTexture0, uv); }
float4 SampleShaderTexture1(float2 uv) { return SampleTexture(gShaderTexture1, uv); }

#endif // MODEL_SURFACE_HLSLI
