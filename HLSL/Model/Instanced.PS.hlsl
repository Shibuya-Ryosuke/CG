Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// このInstancedModel全体で共有する設定(色はインスタンスごとなのでここには無い)
cbuffer InstancedMaterial : register(b0)
{
    int gEnableLighting;
    int gShadingMode; // 0:None 1:Lambert 2:HalfLambert (Geometry.hのShadingModeと対応)
    float2 padding;
};

// C++側 Light構造体(Light.h)と1:1でレイアウトを合わせること。全体で64byte。
struct Light
{
    float4 color;
    float3 direction;
    float intensity;
    float3 position;
    float range;
    float spotAngle;
    float spotFalloff;
    uint type;
    float padding;
};

static const uint LIGHT_TYPE_DIRECTIONAL = 0;
static const uint LIGHT_TYPE_POINT = 1;
static const uint LIGHT_TYPE_SPOT = 2;
static const uint LIGHT_TYPE_AREA = 3;

StructuredBuffer<Light> gLights : register(t1);

cbuffer LightCountBuffer : register(b1)
{
    uint gLightCount;
};

cbuffer AmbientLightBuffer : register(b2)
{
    float4 gAmbientColor;
    float gAmbientIntensity;
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float4 color : COLOR0;
    float3 worldPosition : TEXCOORD1;
};

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// NOTE: Model_PS.hlslのライティング計算とロジックを揃えている(シャドウ判定だけが無い)
float ComputeCosTerm(float3 normal, float3 dirToLight, int shadingMode)
{
    if (shadingMode == 1)
    {
        return saturate(dot(normal, dirToLight));
    }
    else if (shadingMode == 2)
    {
        float NdotL = dot(normal, dirToLight);
        return pow(NdotL * 0.5f + 0.5f, 2.0f);
    }
    return 1.0f;
}

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

    if (gEnableLighting != 0 && gShadingMode != 0)
    {
        float3 normal = normalize(input.normal);
        float3 lightSum = float3(0.0f, 0.0f, 0.0f);

        for (uint i = 0; i < gLightCount; ++i)
        {
            Light light = gLights[i];
            float3 dirToLight = float3(0.0f, 0.0f, 0.0f);
            float attenuation = 1.0f;

            if (light.type == LIGHT_TYPE_DIRECTIONAL)
            {
                dirToLight = -light.direction;
            }
            else if (light.type == LIGHT_TYPE_POINT || light.type == LIGHT_TYPE_SPOT || light.type == LIGHT_TYPE_AREA)
            {
                float3 toLight = light.position - input.worldPosition;
                float distance = length(toLight);
                dirToLight = toLight / max(distance, 0.0001f);

                float rangeAtten = saturate(1.0f - (distance / max(light.range, 0.0001f)));
                attenuation = rangeAtten * rangeAtten;

                if (light.type == LIGHT_TYPE_SPOT)
                {
                    float cosAngle = dot(light.direction, -dirToLight);
                    float spotAtten = saturate((cosAngle - light.spotAngle) / max(light.spotFalloff, 0.0001f));
                    attenuation *= spotAtten;
                }
            }
            else
            {
                continue;
            }

            float cosTerm = ComputeCosTerm(normal, dirToLight, gShadingMode);
            lightSum += light.color.rgb * cosTerm * light.intensity * attenuation;
        }

        lightSum += gAmbientColor.rgb * gAmbientIntensity;

        output.color.rgb = input.color.rgb * textureColor.rgb * lightSum;
        output.color.a = input.color.a * textureColor.a;
    }
    else
    {
        output.color = input.color * textureColor;
    }

    return output;
}
