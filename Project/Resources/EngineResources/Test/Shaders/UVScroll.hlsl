// テスト用のカスタムシェーダー: テクスチャを流す (UVスクロール)
// 自由な値 (Model::SetShaderParam):
//   0番: 横に流れる速さ (1秒あたりのUV)
//   1番: 縦に流れる速さ (1秒あたりのUV)
//   2番: 色を波打たせる強さ (0で無し)
//
// 書き方の決まり (詳しくは Resources/EngineResources/HLSL/Model/ModelSurface.hlsli の先頭):
//   ・Surface() は必須。output には最初から「いつもの見た目」が入っているので、変えたいところだけ書き換える
//   ・頂点を動かしたいときは、#define USE_CUSTOM_VERTEX を書いて Vertex() を作る (このファイルでは使っていない)
//       #define USE_CUSTOM_VERTEX
//       void Vertex(inout VertexInput v, ModelInstance instance)
//       {
//           v.position.y += sin(v.position.x + gTime) * 0.2f;   // ローカル座標で波打たせる
//       }

// エディタ(IntelliSense)で型や関数が分かるようにするための include (無くてもコンパイルは通る)。
// パスはこのファイルから見た ModelSurface.hlsli の場所に合わせること
#include "../../HLSL/Model/ModelSurface.hlsli"

void Surface(SurfaceInput input, inout SurfaceOutput output)
{
    // UVを時間で動かしてからテクスチャを読み直す
    float2 speed = input.params[0].xy;
    float2 uv = input.uv + speed * input.time;
    float4 textureColor = SampleMainTexture(uv);
    output.color = input.instance.color.rgb * textureColor.rgb;

    // 色を少し波打たせる (2番が0なら何もしない)
    float wave = sin(input.worldPosition.x * 0.5f + input.time * 3.0f) * 0.5f + 0.5f;
    output.color *= lerp(1.0f, 0.6f + wave * 0.4f, saturate(input.params[0].z));
}
