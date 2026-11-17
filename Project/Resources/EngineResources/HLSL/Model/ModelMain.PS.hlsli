// モデル描画のピクセルシェーダー main (エンジン側の型紙)
// ShaderManager が「ModelSurface.hlsli → アプリのシェーダー → このファイル」の順にまとめてコンパイルする。
// (旧 ModelBatch.PS.hlsl の処理を、アプリの Surface() を呼ぶ形にしたもの)
//   1. カメラに近いときの処理 (不透明はディザ)          … 必ず通る
//   2. Surface() で色を決める                           … アプリの処理
//   3. ライティング・影                                  … output.enableLighting が true のとき
//      (Phong / BlinnPhong は鏡面反射も足す)
//   4. 発光を足す
//   5. カメラに近いときの処理 (半透明・加算などは薄くする) … 必ず通る
// エディタ(IntelliSense)がこのファイル単体を見ても型や関数が分かるように include しておく。
// 実際のコンパイルでは ShaderManager が先に include しているので、二重には読まれない (include guard)
#include "ModelSurface.hlsli"
// アプリのシェーダー (またはエンジン標準の DefaultSurface.hlsli) で定義される
void Surface(SurfaceInput input, inout SurfaceOutput output);
#include "ModelBatch.hlsli"
#include "Dither.hlsli"

// エディタ用: 単体で見たときだけ標準の Surface を読む (実際のコンパイルでは MODEL_STAGE_PS が定義されているので読まない)
// 定義してないよーってエラーがでてしまうので
#ifndef MODEL_STAGE_PS
#include "DefaultSurface.hlsli"
#endif

// C++側 Light構造体(Light.h)と1:1でレイアウトを合わせること。全体で96byte。
struct Light
{
    float4 color;       // rgb=色, a=未使用
    float3 direction;   // Directional/Spotで使用（正規化済み）
    float intensity;    // 共通：明るさ
    float3 position;    // Point/Spot/Areaで使用
    float range;        // Point/Spotの減衰距離
    float spotAngle;    // Spotの照射角 (cos値)
    float spotFalloff;  // Spotの縁の滑らかさ
    uint type;          // ライト種別 (0:Directional 1:Point 2:Spot 3:Area)
    float padding;      // 16byteアライン調整用
    float3 areaRight;   // Areaで使用：面の横方向 (正規化済み。C++側が毎フレーム計算して入れる)
    float areaWidth;    // Areaで使用：面の幅 (端から端までの長さ)
    float3 areaUp;      // Areaで使用：面の縦方向 (正規化済み)
    float areaHeight;   // Areaで使用：面の高さ (端から端までの長さ)
};

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT       = 1;
static const uint LIGHT_TYPE_SPOT        = 2;
static const uint LIGHT_TYPE_AREA        = 3;


// Light配列本体 (StructuredBuffer。C++側はRoot Descriptor SRVとして直接バインドしている)
StructuredBuffer<Light> gLights : register(t1);

// 現在の有効ライト数
cbuffer LightCountBuffer : register(b1)
{
    uint gLightCount;
};

// アンビエントライト(シーン全体への底上げ光。方向・位置を持たない)
cbuffer AmbientLightBuffer : register(b2)
{
    float4 gAmbientColor;
    float gAmbientIntensity;
};

// ライトのView-Projection行列 (シャドウマップ参照用。C++側のroot param 6、Vertex/Pixel共通)
cbuffer LightViewProjBuffer : register(b3)
{
    float4x4 gLightViewProj;
};

// シャドウマップ本体と、PCF用の比較サンプラー
Texture2D<float> gShadowMap : register(t2);
SamplerComparisonState gShadowSampler : register(s1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

static const int SHADING_LAMBERT      = 1;
static const int SHADING_HALF_LAMBERT = 2;
static const int SHADING_PHONG        = 3;
static const int SHADING_BLINN_PHONG  = 4;

// 法線とライトへの向き(dirToLight：ピクセルからライトの方向)から、Lambert/HalfLambertの係数を計算する
// (Phong / BlinnPhong の拡散反射は Lambert と同じ)
float ComputeCosTerm(float3 normal, float3 dirToLight, int shadingMode)
{
    if (shadingMode == SHADING_LAMBERT || shadingMode == SHADING_PHONG || shadingMode == SHADING_BLINN_PHONG)
    {
        // Lambert
        return saturate(dot(normal, dirToLight));
    }
    else if (shadingMode == 2)
    {
        // Half Lambert
        float NdotL = dot(normal, dirToLight);
        return pow(NdotL * 0.5f + 0.5f, 2.0f);
    }
    return 1.0f;
}

// 鏡面反射(ハイライト)の係数を計算する。Phong / BlinnPhong 以外は0
//   dirToLight : ピクセルからライトへの向き
//   dirToCamera: ピクセルからカメラへの向き
float ComputeSpecularTerm(float3 normal, float3 dirToLight, float3 dirToCamera, float shininess, int shadingMode)
{
    // ライトが面の裏側にあるときはハイライトを出さない (裏まで光が回り込まないように)
    if (dot(normal, dirToLight) <= 0.0f)
    {
        return 0.0f;
    }
    if (shadingMode == SHADING_PHONG)
    {
        // Phong: ライトの光が面で跳ね返る向き(反射ベクトル)と、カメラへの向きがどれだけ近いか
        float3 reflectDir = reflect(-dirToLight, normal);
        return pow(saturate(dot(reflectDir, dirToCamera)), shininess);
    }
    if (shadingMode == SHADING_BLINN_PHONG)
    {
        // Blinn-Phong: ライトとカメラのちょうど中間の向き(ハーフベクトル)と、法線がどれだけ近いか
        float3 halfVector = normalize(dirToLight + dirToCamera);
        return pow(saturate(dot(normal, halfVector)), shininess);
    }
    return 0.0f;
}

// エリアライト(長方形の面光源)の面の上で、指定した座標に一番近い場所を返す
//   面の中心からのずれを、横方向・縦方向の成分に分けて、それぞれ面の端までに収める
float3 ClosestPointOnAreaLight(Light light, float3 targetPosition)
{
    float3 fromCenter = targetPosition - light.position;
    float2 halfSize = float2(light.areaWidth, light.areaHeight) * 0.5f;
    float alongRight = clamp(dot(fromCenter, light.areaRight), -halfSize.x, halfSize.x);
    float alongUp = clamp(dot(fromCenter, light.areaUp), -halfSize.y, halfSize.y);
    return light.position + light.areaRight * alongRight + light.areaUp * alongUp;
}

// ワールド座標から、シャドウマップを使って「影の量」を計算する (1:影なし 〜 0:完全に影)
// NOTE: 現状はDirectionalLight(Light 0)専用。Point/Spotの影は別方式が必要なため未対応。
float CalculateShadowFactor(float3 worldPosition)
{
    float4 lightClipPos = mul(float4(worldPosition, 1.0f), gLightViewProj);
    // 正射影なのでw除算は本来不要だが、念のため行っておく
    float3 lightNDC = lightClipPos.xyz / lightClipPos.w;

    // NDC(-1〜1、Yは上向き)からテクスチャUV(0〜1、Vは下向き)へ変換
    float2 shadowUV;
    shadowUV.x = lightNDC.x * 0.5f + 0.5f;
    shadowUV.y = -lightNDC.y * 0.5f + 0.5f;
    float currentDepth = lightNDC.z;

    // シャドウマップの範囲外(影を落とす対象範囲の外)は、影なし扱いにする
    if (shadowUV.x < 0.0f || shadowUV.x > 1.0f || shadowUV.y < 0.0f || shadowUV.y > 1.0f ||
        currentDepth < 0.0f || currentDepth > 1.0f)
    {
        return 1.0f;
    }

    // SampleCmpLevelZeroでハードウェアPCF(2x2の平均)を使う。
    // ComparisonFunc=LESS_EQUALなので、currentDepth <= シャドウマップの深度 なら1(影なし)に近づく。
    return gShadowMap.SampleCmpLevelZero(gShadowSampler, shadowUV, currentDepth);
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    ModelInstance instance = gInstances[input.instanceId];

    // カメラに極端に近いピクセルを消す (Dither.hlsli)
    // 不透明はディザで消す。それ以外は最後に色・アルファを薄くして消す
    float nearOpacity = ComputeNearCameraOpacity(input.position, gNearFadeStart, gNearFadeEnd);
    if (gBlendMode == BLEND_NONE)
    {
        ApplyNearCameraDither(input.position, nearOpacity);
    }

    // Surface() に渡す値
    SurfaceInput surfaceInput;
    surfaceInput.uv = input.texcoord;
    surfaceInput.normal = normalize(input.normal);
    surfaceInput.worldPosition = input.worldPosition;
    surfaceInput.screenPosition = input.position;
    surfaceInput.params = instance.params;
    surfaceInput.time = gTime;
    surfaceInput.instance = instance;

    // 「いつもの見た目」を入れてから Surface() に渡す (何もしなければエンジン標準と同じ見た目になる)
    float4 textureColor = SampleMainTexture(input.texcoord);
    SurfaceOutput surface;
    surface.color = instance.color.rgb * textureColor.rgb;
    surface.alpha = instance.color.a * textureColor.a;
    surface.emissive = instance.emissive.rgb * instance.emissive.a;
    surface.normal = surfaceInput.normal;
    surface.specular = instance.specular.rgb;
    surface.shininess = max(instance.specular.a, 1.0f);
    surface.enableLighting = (instance.enableLighting != 0 && instance.shadingMode != 0);

    Surface(surfaceInput, surface);

    if (surface.enableLighting)
    {
        float3 normal = normalize(surface.normal);
        float3 lightSum = float3(0.0f, 0.0f, 0.0f);
        // 鏡面反射の合計 (モデルの色・テクスチャはかけずに足す)
        float3 specularSum = float3(0.0f, 0.0f, 0.0f);
        float3 dirToCamera = normalize(gCameraPosition - input.worldPosition);

        for (uint i = 0; i < gLightCount; ++i)
        {
            Light light = gLights[i];

            if (light.type == LIGHT_TYPE_DIRECTIONAL)
            {
                float3 dirToLight = -light.direction;
                float cosTerm = ComputeCosTerm(normal, dirToLight, instance.shadingMode);
                float shadow = CalculateShadowFactor(input.worldPosition);
                lightSum += light.color.rgb * cosTerm * light.intensity * shadow;
                float specularTerm = ComputeSpecularTerm(normal, dirToLight, dirToCamera, surface.shininess, instance.shadingMode);
                specularSum += light.color.rgb * specularTerm * light.intensity * shadow;
            }
            else if (light.type == LIGHT_TYPE_AREA)
            {
                // エリアライト(長方形の面光源)。代表点法: 面の中から「この1点から照らされている」とみなす点を選び、
                // あとはポイントライトと同じように計算する。
                // light.direction は面が光を出す向き。片面だけ光る (裏側は照らさない)

                // --- 拡散反射: 面の上で、このピクセルに一番近い点から照らされている扱いにする ---
                float3 diffusePoint = ClosestPointOnAreaLight(light, input.worldPosition);
                float3 toLightVec = diffusePoint - input.worldPosition;
                float distance = length(toLightVec);
                float3 dirToLight = toLightVec / max(distance, 0.0001f);

                float attenuation = saturate(1.0f - (distance / max(light.range, 0.0001f)));
                attenuation *= attenuation;
                // 面の正面にあるほど明るく、真横で0、裏側は照らさない
                attenuation *= saturate(dot(light.direction, -dirToLight));

                float cosTerm = ComputeCosTerm(normal, dirToLight, instance.shadingMode);
                lightSum += light.color.rgb * cosTerm * light.intensity * attenuation;

                // --- 鏡面反射: 視線が面で跳ね返った先が、ライトの面のどこに当たるかを求めて、そこを光源の位置にする ---
                // (面に当たらないときは、当たるはずだった場所に一番近い面の上の点を使う。ハイライトが面の形に伸びて見える)
                float3 specularPoint = diffusePoint;
                float3 reflectDir = reflect(-dirToCamera, normal);
                // 跳ね返った視線が、ライトの面の表側へ向かって進んでいるときだけ交点を求める
                float towardLight = dot(reflectDir, light.direction);
                if (towardLight < -0.0001f)
                {
                    float hitDistance = dot(light.position - input.worldPosition, light.direction) / towardLight;
                    if (hitDistance > 0.0f)
                    {
                        specularPoint = ClosestPointOnAreaLight(light, input.worldPosition + reflectDir * hitDistance);
                    }
                }
                float3 dirToSpecularPoint = normalize(specularPoint - input.worldPosition);
                float specularTerm = ComputeSpecularTerm(normal, dirToSpecularPoint, dirToCamera, surface.shininess, instance.shadingMode);
                specularSum += light.color.rgb * specularTerm * light.intensity * attenuation;
            }
            else
            {
                float3 toLightVec = light.position - input.worldPosition;
                float distance = length(toLightVec);
                float3 dirToLight = toLightVec / max(distance, 0.0001f);

                float attenuation = saturate(1.0f - (distance / max(light.range, 0.0001f)));
                attenuation *= attenuation;

                if (light.type == LIGHT_TYPE_SPOT)
                {
                    float cosAngle = dot(-dirToLight, normalize(light.direction));
                    float spotAttenuation = saturate((cosAngle - light.spotAngle) / max(light.spotFalloff, 0.0001f));
                    attenuation *= spotAttenuation;
                }

                float cosTerm = ComputeCosTerm(normal, dirToLight, instance.shadingMode);
                lightSum += light.color.rgb * cosTerm * light.intensity * attenuation;
                float specularTerm = ComputeSpecularTerm(normal, dirToLight, dirToCamera, surface.shininess, instance.shadingMode);
                specularSum += light.color.rgb * specularTerm * light.intensity * attenuation;
            }
        }

        lightSum += gAmbientColor.rgb * gAmbientIntensity;

        output.color.rgb = surface.color * lightSum + surface.specular * specularSum;
    }
    else
    {
        output.color.rgb = surface.color;
    }
    output.color.a = surface.alpha;

    // 発光: ライティングを通さず無条件で加算
    output.color.rgb += surface.emissive;

    // カメラに近い半透明・加算などを、ブレンドモードごとに「何も変えない色」へ近づけて消す
    if (gBlendMode == BLEND_MULTIPLY)
    {
        // 乗算は白(1)が「変化なし」
        output.color.rgb = lerp(float3(1.0f, 1.0f, 1.0f), output.color.rgb, nearOpacity);
    }
    else if (gBlendMode == BLEND_SCREEN)
    {
        // スクリーンは黒(0)が「変化なし」
        output.color.rgb *= nearOpacity;
    }
    else if (gBlendMode != BLEND_NONE)
    {
        // Normal / Add / Subtract はアルファで強さが決まる
        output.color.a *= nearOpacity;
    }

    return output;
}
