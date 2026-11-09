// パーティクル (GPUパーティクル) の共通定義。
// C++側 (Engine/Graphics/Particle/ParticleTypes.h) と、構造体の並び・enumの値を1:1で合わせること。

// ---------------------------------------------------------------------
// 1粒ぶんのデータ (ParticleGPUData と同じ並び。全体で112byte)
// ---------------------------------------------------------------------
struct ParticleData
{
    float3 position;
    float  scale;          // 発生時の大きさ (見た目の倍率は描画側で掛ける)
    float3 velocity;
    float  totalLife;      // 発生時の寿命(秒)
    float4 color;          // 発生時の色
    float3 start;          // 収束: 目標からの相対位置 / カーブ: 発生位置
    float  remainingLife;  // 残り寿命(秒)。0以下で死んでいる
    float3 control;        // カーブ(ベジェ)の制御点
    float  seed;           // 0〜1の乱数 (粒ごとの個性に使う)
    float3 target;         // 目標座標 (収束・渦・追尾・カーブ・吸い込み)
    float  rotation;       // ロール角(ラジアン)
    float  rotationSpeed;  // ロールの角速度(ラジアン/秒)
    float  blinkFrequency; // 点滅の周波数(回/秒)
    uint   flags;          // kFlagChild など
    float  padding;
};

static const uint kFlagChild = 1u; // 分裂で生まれた子 (もう分裂しない・物理で動く)

// ---------------------------------------------------------------------
// 発生の命令 (ParticleSpawnBatch と同じ並び。80byte)
// CPUは「どこに何粒」だけを送り、1粒ずつの初期値はシミュレーションのシェーダーが作る
// ---------------------------------------------------------------------
struct SpawnBatch
{
    float3 origin;      // 発生位置
    uint   count;       // 何粒出すか
    float3 target;      // 目標
    uint   firstIndex;  // このフレームの全命令を並べたときの、先頭の通し番号
    float3 axis;        // 輪・円柱の軸 (正規化済み)
    float  radiusScale; // 発生半径の縮み
    float3 tangent1;    // 軸に直交する2本
    uint   seed;        // 乱数の種
    float3 tangent2;
    float  padding;
};

// 発生する場所の形 (C++側 ParticleSpawnShape と同じ値)
static const uint kShapePoint    = 0u;
static const uint kShapeSphere   = 1u;
static const uint kShapeRing     = 2u;
static const uint kShapeCylinder = 3u;
static const uint kShapeLine     = 4u;
static const uint kShapeBox      = 5u;

// 初速の決め方 (C++側 ParticleVelocityMode と同じ値)
static const uint kVelocityFixed   = 0u;
static const uint kVelocityRandom  = 1u;
static const uint kVelocityRadial  = 2u;
static const uint kVelocityTangent = 3u;

static const float kTwoPi = 6.28318530718f;

// 動き方 (C++側 ParticleMotion と同じ値)
static const uint kMotionPhysics  = 0u;
static const uint kMotionExplode  = 1u;
static const uint kMotionConverge = 2u;
static const uint kMotionVortex   = 3u;
static const uint kMotionHoming   = 4u;
static const uint kMotionBezier   = 5u;

// 消え方 (C++側 ParticleDeath と同じ値)
static const uint kDeathFade       = 0u;
static const uint kDeathBlink      = 1u;
static const uint kDeathShrink     = 2u;
static const uint kDeathStretch    = 3u;
static const uint kDeathColorShift = 4u;
static const uint kDeathSplit      = 5u;
static const uint kDeathAbsorb     = 6u;

// 1粒の形 (C++側 ParticleLook と同じ値)
static const uint kLookTexture    = 0u;
static const uint kLookSoftCircle = 1u;
static const uint kLookSpark      = 2u;
static const uint kLookRing       = 3u;
static const uint kLookStar       = 4u;

// ---------------------------------------------------------------------
// 共通の関数
// ---------------------------------------------------------------------

float Hash11(float n)
{
    return frac(sin(n) * 43758.5453f);
}

float3 Hash31(float n)
{
    return frac(sin(float3(n, n + 1.31f, n + 2.71f)) * float3(43758.5453f, 22578.1459f, 19642.3490f));
}

// PCGハッシュ (整数 → 散らばった整数)。GPUで乱数を作るのに使う
uint PcgHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// state を進めて 0〜1 の乱数を返す
float NextRandom(inout uint state)
{
    state = PcgHash(state);
    return (float)(state >> 8u) * (1.0f / 16777216.0f);
}

float RandomRange(inout uint state, float a, float b)
{
    return lerp(a, b, NextRandom(state));
}

float3 RandomInBox(inout uint state, float3 a, float3 b)
{
    float3 t;
    t.x = NextRandom(state);
    t.y = NextRandom(state);
    t.z = NextRandom(state);
    return lerp(a, b, t);
}

float3 RandomUnitVector(inout uint state)
{
    float z = RandomRange(state, -1.0f, 1.0f);
    float angle = NextRandom(state) * kTwoPi;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    return float3(r * cos(angle), r * sin(angle), z);
}

// 消え始めてからの進み具合 (0:まだ消え始めていない 〜 1:寿命の終わり)
// deathRatio = 寿命の最後の何割で消える演出をするか
float DeathProgress(float remainingLife, float totalLife, float deathRatio)
{
    if (deathRatio <= 0.0001f)
    {
        return 0.0f;
    }
    float lifeRatio = saturate(remainingLife / max(totalLife, 0.0001f));
    return saturate((deathRatio - lifeRatio) / deathRatio);
}

// axis(正規化済み)まわりに angle(ラジアン) 回す (ロドリゲスの回転公式)
float3 RotateAroundAxis(float3 v, float3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0f - c);
}

// 発散しない(湧き出し・吸い込みの無い)揺らぎの流れ場。
// 各成分が「自分以外の座標」だけに依存するので、div = 0 になる (カールノイズの簡易版)
float3 FlowField(float3 p, float t)
{
    return float3(
        sin(p.y * 1.7f + t) + sin(p.z * 1.3f - t * 0.7f),
        sin(p.z * 1.5f + t * 1.1f) + sin(p.x * 1.9f - t * 0.5f),
        sin(p.x * 1.4f + t * 0.9f) + sin(p.y * 1.2f - t * 1.3f)) * 0.5f;
}

// ---------------------------------------------------------------------
// シミュレーション用パラメータ (ParticleSimParams と同じ並び)
// ---------------------------------------------------------------------
#ifdef PARTICLE_FX_SIMULATION
cbuffer SimParams : register(b0)
{
    float  gDeltaTime;
    uint   gSpawnCount;        // 今フレームに発生させる総数 (全命令の合計)
    uint   gMaxCount;          // スロット総数
    uint   gMotion;

    float  gGravity;
    float  gDrag;
    float  gNoiseStrength;
    float  gNoiseFrequency;

    float3 gLiveTarget;        // 毎フレーム動く目標 (追従している場合)
    uint   gUseLiveTarget;     // 1: gLiveTarget を使う / 0: 粒ごとの target を使う

    float3 gAxis;              // 回転軸 (収束の弧・渦)。正規化済み
    float  gArcAngle;          // 収束: 到着までに回る角度(ラジアン)

    float  gEasePower;         // 収束・カーブ: 加速の強さ (1で等速、大きいほど最後に一気に来る)
    float  gHomingSpeed;
    float  gHomingTurn;        // 追尾: 曲がりやすさ
    float  gArriveRadius;      // 追尾: この距離まで来たら消える

    float  gVortexAngularSpeed; // 渦: 角速度(ラジアン/秒)
    float  gVortexRadialSpeed;  // 渦: 半径の変化(負で吸い込まれる)
    float  gVortexRiseSpeed;    // 渦: 軸方向の速さ
    uint   gArcBothDirections;  // 収束: 1なら半分の粒は逆回り

    uint   gBounce;
    float  gFloorY;
    float  gRestitution;
    float  gFriction;

    uint   gDeath;
    float  gDeathRatio;
    float  gAbsorbStrength;
    float  gTime;

    uint   gSplitCount;
    float  gSplitSpeed;
    float  gSplitLife;
    float  gSplitScale;

    uint   gChildReadSlot;     // 前フレームに書かれた「分裂の子」のリスト (0/1)
    uint   gClear;             // 1: 全部消す
    uint   gBatchCount;        // 今フレームの発生の命令の数
    float  gSimPadding1;
};

// 発生のさせ方 (ParticleSpawnParams と同じ並び)。1粒の初期値を作るのに使う
cbuffer SpawnParams : register(b1)
{
    uint   gShape;
    float  gRadius;
    float  gInnerRatio;
    float  gHeight;

    float3 gBoxSize;
    uint   gVelocityMode;

    float3 gLineEnd;
    uint   gRandomRotation;

    float3 gVelocity;
    float  gSpeedMin;

    float3 gVelocityMin;
    float  gSpeedMax;

    float3 gVelocityMax;
    float  gLifeMin;

    float3 gControlOffsetMin;
    float  gLifeMax;

    float3 gControlOffsetMax;
    float  gScaleMin;

    float4 gColorA;
    float4 gColorB;

    float  gScaleMax;
    float  gRotationSpeedMin;  // ラジアン/秒
    float  gRotationSpeedMax;
    float  gBlinkFrequencyMin;

    float  gBlinkFrequencyMax;
    float  gSpawnPadding0;
    float  gSpawnPadding1;
    float  gSpawnPadding2;
};
#endif

// ---------------------------------------------------------------------
// 描画用パラメータ (ParticleRenderParams と同じ並び)
// ---------------------------------------------------------------------
#ifdef PARTICLE_FX_RENDER
cbuffer RenderParams : register(b0)
{
    float4x4 gViewProjection;
    float3   gCameraRight;
    float    gTime;
    float3   gCameraUp;
    uint     gDeath;
    float4   gEndColor;        // 寿命の終わりに色へ掛ける値 (白なら変化なし)
    float4   gDeathColor;      // 色変化で消えるときの途中の色
    float    gStartScale;      // 寿命に対する大きさの倍率 (始め)
    float    gEndScale;        // 寿命に対する大きさの倍率 (終わり)
    float    gDeathRatio;
    uint     gLook;
    float    gStretch;         // 速度方向へ伸ばす量
    float    gSpeedBrightness; // 速いほど明るくする量
    float    gJitterStrength;  // ジッター(カクッとずれる)の大きさ
    float    gJitterFrequency; // ジッターの回数(回/秒)
    float    gBlinkFade;       // 点滅中に一緒に薄くしていく量 (0〜1)
    float    gIntensity;       // 明るさ (1より大きいとブルームで光る)
    float    gDeathStretch;    // 伸びて消えるときの伸び量
    float    gSoftness;        // 形のふちのぼけ具合
};

struct VertexShaderOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    float2 local : TEXCOORD1;  // 板の中の位置 (-1〜1)。形をプログラムで描くのに使う
    float4 color : COLOR0;
};
#endif
