#pragma once
#include "../../Core/Math/Math.h"
#include <cstdint>
#include <string>
#include <vector>

namespace RyoEngine {

    // =====================================================================
    // 種類 (HLSL側 Particle.hlsli の値と合わせること)
    // =====================================================================

    /// <summary>発生のさせ方</summary>
    enum class ParticleEmitMode : uint32_t {
        Burst,      // 一度にまとめて (回数・間隔を指定して繰り返せる)
        Continuous, // 毎秒N個ずつ
        Count,
    };

    /// <summary>発生する場所の形</summary>
    enum class ParticleSpawnShape : uint32_t {
        Point,    // 1点
        Sphere,   // 球 (innerRatio=1で表面だけ、0で中まで)
        Ring,     // 輪 (innerRatio=1で線、0で円盤)
        Cylinder, // 円柱の側面
        Line,     // 線分
        Box,      // 箱
        Count,
    };

    /// <summary>初速の決め方 (自由・追尾で使う)</summary>
    enum class ParticleVelocityMode : uint32_t {
        Fixed,   // 固定値
        Random,  // 箱の範囲で乱数
        Radial,  // 中心から外向き
        Tangent, // 軸のまわりを回る向き
        Count,
    };

    /// <summary>動き方</summary>
    enum class ParticleMotion : uint32_t {
        Physics,  // 自由 (重力・空気抵抗・揺らぎ・バウンド)
        Explode,  // 放射 (中心から外向きに飛んで減速)
        Converge, // 収束 (外側から弧を描いて目標へ)
        Vortex,   // 渦 (軸のまわりを回りながら上昇・下降)
        Homing,   // 追尾 (目標へ曲がりながら向かう)
        Bezier,   // カーブ (発生位置→制御点→目標)
        Count,
    };

    /// <summary>消え方</summary>
    enum class ParticleDeath : uint32_t {
        Fade,       // 薄くなる
        Blink,      // 点滅の幅を減らしながら消える
        Shrink,     // 縮む (1→0)
        Stretch,    // 進行方向に伸びながら細くなる
        ColorShift, // 色が変わりながら暗くなる
        Split,      // 寿命の終わりに分裂する
        Absorb,     // 目標へ吸い込まれる
        Count,
    };

    /// <summary>1粒の形 (プログラムで描く。Textureだけ画像を使う)</summary>
    enum class ParticleLook : uint32_t {
        Texture,
        SoftCircle,
        Spark,
        Ring,
        Star,
        Count,
    };

    enum class ParticleBlend : uint32_t {
        Alpha,
        Additive,
        Count,
    };

    /// <summary>
    /// 表示名(ImGui)とJSONのキー名
    /// </summary>
    struct ParticleNames {
        static constexpr const char* kEmitModeLabels[] = { "まとめて(Burst)", "連続(Continuous)" };
        static constexpr const char* kEmitModeIds[] = { "Burst", "Continuous" };

        static constexpr const char* kSpawnShapeLabels[] = { "点", "球", "輪", "円柱", "線", "箱" };
        static constexpr const char* kSpawnShapeIds[] = { "Point", "Sphere", "Ring", "Cylinder", "Line", "Box" };

        static constexpr const char* kVelocityModeLabels[] = { "固定", "乱数(箱)", "外向き", "軸まわり" };
        static constexpr const char* kVelocityModeIds[] = { "Fixed", "Random", "Radial", "Tangent" };

        static constexpr const char* kMotionLabels[] = { "自由(物理)", "放射(爆発)", "収束(弧)", "渦", "追尾", "カーブ(ベジェ)" };
        static constexpr const char* kMotionIds[] = { "Physics", "Explode", "Converge", "Vortex", "Homing", "Bezier" };

        static constexpr const char* kDeathLabels[] = { "フェード", "点滅", "縮む", "伸びて細く", "色変化", "分裂", "吸い込み" };
        static constexpr const char* kDeathIds[] = { "Fade", "Blink", "Shrink", "Stretch", "ColorShift", "Split", "Absorb" };

        static constexpr const char* kLookLabels[] = { "画像", "丸(ぼかし)", "火花(ひし形)", "輪", "星" };
        static constexpr const char* kLookIds[] = { "Texture", "SoftCircle", "Spark", "Ring", "Star" };

        static constexpr const char* kBlendLabels[] = { "通常(Alpha)", "加算(Additive)" };
        static constexpr const char* kBlendIds[] = { "Alpha", "Additive" };
    };

    // =====================================================================
    // 連鎖：このエフェクトを再生してから delay 秒後に、別のエフェクトを再生する
    // (終わってからつなぐなら delay = このエフェクトの長さ、途中から重ねるなら短くする)
    // =====================================================================
    struct ParticleFollowUp {
        std::string effectName;   // 再生するエフェクトの名前
        float delay = 0.0f;       // 再生してから何秒後か
        bool samePosition = true; // true: このエフェクトの発生位置で再生 / false: 呼ばれた側の設定の位置で再生
    };

    // =====================================================================
    // 1つのエフェクトの設定 (エディタでの編集対象・JSONの保存対象)
    // =====================================================================
    struct ParticleConfig {
        // --- 発生 ---
        ParticleEmitMode emitMode = ParticleEmitMode::Burst;
        uint32_t maxParticles = 1024;  // 同時に存在できる最大数 (GPUバッファの大きさ)
        uint32_t burstCount = 100;     // まとめて: 1回の数
        uint32_t burstRepeat = 1;      // まとめて: 何回出すか
        float burstInterval = 0.2f;    // まとめて: 繰り返しの間隔(秒)
        float rate = 100.0f;           // 連続: 1秒あたりの数
        float duration = 1.0f;         // 連続: 出し続ける時間(秒)。0以下なら Stop するまで
        bool useUnscaledTime = false;  // true: スロー(タイムスケール)の影響を受けない

        // --- 位置・連携 (BindingRegistry に登録されたキーを選ぶ) ---
        std::string positionKey;       // 発生位置を追従する座標のキー (空なら Play の位置 or basePosition)
        RyoEngine::Vector3 basePosition = { 0.0f, 0.0f, 0.0f };
        RyoEngine::Vector3 positionOffset = { 0.0f, 0.0f, 0.0f };
        std::string targetKey;         // 目標を追従する座標のキー (空なら 発生位置 + targetOffset)
        RyoEngine::Vector3 targetOffset = { 0.0f, 0.0f, 0.0f };
        std::string triggerFlagKey;    // このフラグが立った瞬間に Play する
        bool stopWhenFlagOff = true;   // フラグが下りたら Stop する (連続のとき)

        // --- 発生する場所の形 ---
        ParticleSpawnShape spawnShape = ParticleSpawnShape::Point;
        float radius = 1.0f;
        float innerRatio = 1.0f;       // 球・輪・円柱: 1で外周だけ、0で中まで
        float height = 1.0f;           // 円柱の高さ
        RyoEngine::Vector3 boxSize = { 1.0f, 1.0f, 1.0f };
        RyoEngine::Vector3 lineEnd = { 2.0f, 0.0f, 0.0f }; // 線: 発生位置からの終点
        RyoEngine::Vector3 shapeAxis = { 0.0f, 1.0f, 0.0f }; // 輪・円柱の軸
        float shrinkTime = 0.0f;       // 発生半径が縮むまでの時間(秒)。0なら縮まない
        float shrinkEndRatio = 0.2f;   // 縮み終わったときの半径の割合

        // --- 初速 ---
        ParticleVelocityMode velocityMode = ParticleVelocityMode::Random;
        RyoEngine::Vector3 velocity = { 0.0f, 1.0f, 0.0f };
        RyoEngine::Vector3 velocityMin = { -1.0f, 1.0f, -1.0f };
        RyoEngine::Vector3 velocityMax = { 1.0f, 2.0f, 1.0f };
        float speedMin = 2.0f;         // 外向き・軸まわり・放射の速さ
        float speedMax = 4.0f;

        // --- 寿命・大きさ・回転 ---
        float lifeMin = 0.8f;
        float lifeMax = 1.2f;
        float scaleMin = 0.1f;
        float scaleMax = 0.2f;
        float startScale = 1.0f;       // 寿命に対する大きさの倍率 (始め)
        float endScale = 1.0f;         // 寿命に対する大きさの倍率 (終わり)
        bool randomRotation = true;
        float rotationSpeedMin = 0.0f; // 度/秒
        float rotationSpeedMax = 0.0f;

        // --- 動き方 ---
        ParticleMotion motion = ParticleMotion::Physics;
        float gravity = 0.0f;
        float drag = 0.0f;             // 空気抵抗 (大きいほどすぐ止まる)
        float noiseStrength = 0.0f;    // 揺らぎの強さ
        float noiseFrequency = 1.0f;   // 揺らぎの細かさ
        RyoEngine::Vector3 motionAxis = { 0.0f, 1.0f, 0.0f }; // 収束の弧・渦の軸
        bool axisFacesCamera = false;  // true: 輪・円柱の軸と動きの軸をカメラの向きにする (画面に正対した輪になる)
        float arcAngle = 180.0f;       // 収束: 到着までに回る角度(度)
        bool arcBothDirections = false;// 収束: 半分の粒は逆回り
        float easePower = 2.0f;        // 収束・カーブ: 1で等速、大きいほど最後に一気に来る
        float homingSpeed = 12.0f;
        float homingTurn = 4.0f;
        float arriveRadius = 0.3f;
        float vortexAngularSpeed = 180.0f; // 度/秒
        float vortexRadialSpeed = 0.0f;
        float vortexRiseSpeed = 1.0f;
        RyoEngine::Vector3 controlOffsetMin = { -3.0f, 2.0f, -3.0f }; // カーブ: 中点からの制御点のずれ
        RyoEngine::Vector3 controlOffsetMax = { 3.0f, 5.0f, 3.0f };
        bool bounce = false;
        float floorY = 0.0f;
        float restitution = 0.5f;      // 跳ね返りの強さ
        float friction = 0.8f;         // 跳ねたときの横方向の減速
        float jitterStrength = 0.0f;   // ジッターの大きさ
        float jitterFrequency = 20.0f; // ジッターの回数(回/秒)

        // --- 消え方 ---
        ParticleDeath death = ParticleDeath::Fade;
        float deathRatio = 0.3f;       // 寿命の最後の何割で消える演出をするか
        float blinkFrequencyMin = 8.0f;
        float blinkFrequencyMax = 20.0f;
        float blinkFade = 0.5f;        // 点滅しながら一緒に薄くする量
        float deathStretch = 3.0f;     // 伸びて消えるときの伸び量
        RyoEngine::Vector4 deathColor = { 1.0f, 0.3f, 0.05f, 1.0f };
        uint32_t splitCount = 6;
        float splitSpeed = 3.0f;
        float splitLife = 0.5f;
        float splitScale = 0.5f;
        float absorbStrength = 20.0f;

        // --- 見た目 ---
        ParticleLook look = ParticleLook::SoftCircle;
        std::string texturePath;       // 画像のとき (名前でもパスでもよい)
        ParticleBlend blend = ParticleBlend::Additive;
        RyoEngine::Vector4 colorA = { 1.0f, 1.0f, 1.0f, 1.0f }; // 発生時の色は colorA〜colorB の間で乱数
        RyoEngine::Vector4 colorB = { 1.0f, 1.0f, 1.0f, 1.0f };
        RyoEngine::Vector4 endColor = { 1.0f, 1.0f, 1.0f, 1.0f }; // 寿命の終わりに色へ掛ける値
        float intensity = 1.0f;        // 明るさ (1より大きいとブルームで光る)
        float softness = 2.0f;         // 形のふちのぼけ具合
        float stretch = 0.0f;          // 速度方向への伸び
        float speedBrightness = 0.0f;  // 速いほど明るくする量

        // --- 連鎖 ---
        std::vector<ParticleFollowUp> followUps;
    };

    // =====================================================================
    // GPUへ渡すデータ (HLSL側と1:1でレイアウトを合わせること)
    // =====================================================================

    // 1粒ぶん。HLSLの FxParticle と同じ並び
    struct ParticleGPUData {
        RyoEngine::Vector3 position;
        float scale;
        RyoEngine::Vector3 velocity;
        float totalLife;
        RyoEngine::Vector4 color;
        RyoEngine::Vector3 start;
        float remainingLife;
        RyoEngine::Vector3 control;
        float seed;
        RyoEngine::Vector3 target;
        float rotation;
        float rotationSpeed;
        float blinkFrequency;
        uint32_t flags;
        float padding;
    };
    static_assert(sizeof(ParticleGPUData) == 112, "ParticleGPUData のサイズがシェーダー側と食い違うと壊れるので確認すること");

    // シミュレーション用 (HLSLの SimParams と同じ並び)
    struct ParticleSimParams {
        float deltaTime;
        uint32_t spawnCount;
        uint32_t maxCount;
        uint32_t motion;

        float gravity;
        float drag;
        float noiseStrength;
        float noiseFrequency;

        RyoEngine::Vector3 liveTarget;
        uint32_t useLiveTarget;

        RyoEngine::Vector3 axis;
        float arcAngle;

        float easePower;
        float homingSpeed;
        float homingTurn;
        float arriveRadius;

        float vortexAngularSpeed;
        float vortexRadialSpeed;
        float vortexRiseSpeed;
        uint32_t arcBothDirections;

        uint32_t bounce;
        float floorY;
        float restitution;
        float friction;

        uint32_t death;
        float deathRatio;
        float absorbStrength;
        float time;

        uint32_t splitCount;
        float splitSpeed;
        float splitLife;
        float splitScale;

        uint32_t childReadSlot;
        uint32_t clear;
        uint32_t batchCount;
        float padding1;
    };
    static_assert(sizeof(ParticleSimParams) == 160, "ParticleSimParams のサイズがシェーダー側と食い違うと壊れるので確認すること");

    // 発生の命令 (HLSLの SpawnBatch と同じ並び)。CPUは「どこに何粒」だけを送る
    struct ParticleSpawnBatch {
        RyoEngine::Vector3 origin;
        uint32_t count;
        RyoEngine::Vector3 target;
        uint32_t firstIndex;  // ParticleEmitter::Update で詰める
        RyoEngine::Vector3 axis;
        float radiusScale;
        RyoEngine::Vector3 tangent1;
        uint32_t seed;
        RyoEngine::Vector3 tangent2;
        float padding;
    };
    static_assert(sizeof(ParticleSpawnBatch) == 80, "ParticleSpawnBatch のサイズがシェーダー側と食い違うと壊れるので確認すること");

    // 発生のさせ方 (HLSLの SpawnParams と同じ並び)。1粒の初期値をGPUで作るのに使う
    struct ParticleSpawnParams {
        uint32_t shape;
        float radius;
        float innerRatio;
        float height;

        RyoEngine::Vector3 boxSize;
        uint32_t velocityMode;

        RyoEngine::Vector3 lineEnd;
        uint32_t randomRotation;

        RyoEngine::Vector3 velocity;
        float speedMin;

        RyoEngine::Vector3 velocityMin;
        float speedMax;

        RyoEngine::Vector3 velocityMax;
        float lifeMin;

        RyoEngine::Vector3 controlOffsetMin;
        float lifeMax;

        RyoEngine::Vector3 controlOffsetMax;
        float scaleMin;

        RyoEngine::Vector4 colorA;
        RyoEngine::Vector4 colorB;

        float scaleMax;
        float rotationSpeedMin; // ラジアン/秒
        float rotationSpeedMax;
        float blinkFrequencyMin;

        float blinkFrequencyMax;
        float padding0;
        float padding1;
        float padding2;
    };
    static_assert(sizeof(ParticleSpawnParams) == 192, "ParticleSpawnParams のサイズがシェーダー側と食い違うと壊れるので確認すること");

    // 描画用 (HLSLの RenderParams と同じ並び)
    struct ParticleRenderParams {
        RyoEngine::Matrix4x4 viewProjection;
        RyoEngine::Vector3 cameraRight;
        float time;
        RyoEngine::Vector3 cameraUp;
        uint32_t death;
        RyoEngine::Vector4 endColor;
        RyoEngine::Vector4 deathColor;
        float startScale;
        float endScale;
        float deathRatio;
        uint32_t look;
        float stretch;
        float speedBrightness;
        float jitterStrength;
        float jitterFrequency;
        float blinkFade;
        float intensity;
        float deathStretch;
        float softness;
    };
    static_assert(sizeof(ParticleRenderParams) == 176, "ParticleRenderParams のサイズがシェーダー側と食い違うと壊れるので確認すること");
}
