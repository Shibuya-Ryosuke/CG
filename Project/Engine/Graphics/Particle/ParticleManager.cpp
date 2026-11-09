#include "ParticleManager.h"
#include "ParticleCommon.h"
#include "ParticleTemplates.h"
#include <RyoEngine.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <numbers>
#include <system_error>
#include <utility>

#ifdef NRELEASE
#include <imgui.h>
#endif

#pragma warning(disable: 4866)
#include <json.hpp>

namespace RyoEngine {

    namespace {
        using json = nlohmann::json;

        float DegToRad(float degree) {
            return degree * std::numbers::pi_v<float> / 180.0f;
        }

        // --- JSON ---
        json ToJson(const Vector3& v) { return json::array({ v.x, v.y, v.z }); }
        json ToJson(const Vector4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

        // キーが無い・形が違う場合は defaultValue を返す (古いファイルや手書きの欠けにも強くするため)
        Vector3 ReadVector3(const json& j, const char* key, const Vector3& defaultValue) {
            auto it = j.find(key);
            if (it == j.end() || !it->is_array() || it->size() < 3) {
                return defaultValue;
            }
            Vector3 v{};
            v.x = (*it)[0].get<float>();
            v.y = (*it)[1].get<float>();
            v.z = (*it)[2].get<float>();
            return v;
        }

        Vector4 ReadVector4(const json& j, const char* key, const Vector4& defaultValue) {
            auto it = j.find(key);
            if (it == j.end() || !it->is_array() || it->size() < 4) {
                return defaultValue;
            }
            Vector4 v{};
            v.x = (*it)[0].get<float>();
            v.y = (*it)[1].get<float>();
            v.z = (*it)[2].get<float>();
            v.w = (*it)[3].get<float>();
            return v;
        }

        // enum は数字ではなく名前で保存する (JSONを手で書き換えやすいように)
        template <typename E, size_t N>
        std::string EnumToId(E value, const char* const (&ids)[N]) {
            const size_t index = static_cast<size_t>(value);
            return (index < N) ? ids[index] : ids[0];
        }

        template <typename E, size_t N>
        E ReadEnum(const json& j, const char* key, E defaultValue, const char* const (&ids)[N]) {
            auto it = j.find(key);
            if (it == j.end() || !it->is_string()) {
                return defaultValue;
            }
            const std::string id = it->get<std::string>();
            for (size_t i = 0; i < N; ++i) {
                if (id == ids[i]) {
                    return static_cast<E>(i);
                }
            }
            return defaultValue;
        }

        json ConfigToJson(const ParticleConfig& c) {
            using N = ParticleNames;
            json j;
            // 発生
            j["emitMode"] = EnumToId(c.emitMode, N::kEmitModeIds);
            j["maxParticles"] = c.maxParticles;
            j["burstCount"] = c.burstCount;
            j["burstRepeat"] = c.burstRepeat;
            j["burstInterval"] = c.burstInterval;
            j["rate"] = c.rate;
            j["duration"] = c.duration;
            j["useUnscaledTime"] = c.useUnscaledTime;
            // 位置・連携
            j["positionKey"] = c.positionKey;
            j["basePosition"] = ToJson(c.basePosition);
            j["positionOffset"] = ToJson(c.positionOffset);
            j["targetKey"] = c.targetKey;
            j["targetOffset"] = ToJson(c.targetOffset);
            j["triggerFlagKey"] = c.triggerFlagKey;
            j["stopWhenFlagOff"] = c.stopWhenFlagOff;
            // 形
            j["spawnShape"] = EnumToId(c.spawnShape, N::kSpawnShapeIds);
            j["radius"] = c.radius;
            j["innerRatio"] = c.innerRatio;
            j["height"] = c.height;
            j["boxSize"] = ToJson(c.boxSize);
            j["lineEnd"] = ToJson(c.lineEnd);
            j["shapeAxis"] = ToJson(c.shapeAxis);
            j["shrinkTime"] = c.shrinkTime;
            j["shrinkEndRatio"] = c.shrinkEndRatio;
            // 初速
            j["velocityMode"] = EnumToId(c.velocityMode, N::kVelocityModeIds);
            j["velocity"] = ToJson(c.velocity);
            j["velocityMin"] = ToJson(c.velocityMin);
            j["velocityMax"] = ToJson(c.velocityMax);
            j["speedMin"] = c.speedMin;
            j["speedMax"] = c.speedMax;
            // 寿命・大きさ・回転
            j["lifeMin"] = c.lifeMin;
            j["lifeMax"] = c.lifeMax;
            j["scaleMin"] = c.scaleMin;
            j["scaleMax"] = c.scaleMax;
            j["startScale"] = c.startScale;
            j["endScale"] = c.endScale;
            j["randomRotation"] = c.randomRotation;
            j["rotationSpeedMin"] = c.rotationSpeedMin;
            j["rotationSpeedMax"] = c.rotationSpeedMax;
            // 動き方
            j["motion"] = EnumToId(c.motion, N::kMotionIds);
            j["gravity"] = c.gravity;
            j["drag"] = c.drag;
            j["noiseStrength"] = c.noiseStrength;
            j["noiseFrequency"] = c.noiseFrequency;
            j["motionAxis"] = ToJson(c.motionAxis);
            j["axisFacesCamera"] = c.axisFacesCamera;
            j["arcAngle"] = c.arcAngle;
            j["arcBothDirections"] = c.arcBothDirections;
            j["easePower"] = c.easePower;
            j["homingSpeed"] = c.homingSpeed;
            j["homingTurn"] = c.homingTurn;
            j["arriveRadius"] = c.arriveRadius;
            j["vortexAngularSpeed"] = c.vortexAngularSpeed;
            j["vortexRadialSpeed"] = c.vortexRadialSpeed;
            j["vortexRiseSpeed"] = c.vortexRiseSpeed;
            j["controlOffsetMin"] = ToJson(c.controlOffsetMin);
            j["controlOffsetMax"] = ToJson(c.controlOffsetMax);
            j["bounce"] = c.bounce;
            j["floorY"] = c.floorY;
            j["restitution"] = c.restitution;
            j["friction"] = c.friction;
            j["jitterStrength"] = c.jitterStrength;
            j["jitterFrequency"] = c.jitterFrequency;
            // 消え方
            j["death"] = EnumToId(c.death, N::kDeathIds);
            j["deathRatio"] = c.deathRatio;
            j["blinkFrequencyMin"] = c.blinkFrequencyMin;
            j["blinkFrequencyMax"] = c.blinkFrequencyMax;
            j["blinkFade"] = c.blinkFade;
            j["deathStretch"] = c.deathStretch;
            j["deathColor"] = ToJson(c.deathColor);
            j["splitCount"] = c.splitCount;
            j["splitSpeed"] = c.splitSpeed;
            j["splitLife"] = c.splitLife;
            j["splitScale"] = c.splitScale;
            j["absorbStrength"] = c.absorbStrength;
            // 見た目
            j["look"] = EnumToId(c.look, N::kLookIds);
            j["texturePath"] = c.texturePath;
            j["blend"] = EnumToId(c.blend, N::kBlendIds);
            j["colorA"] = ToJson(c.colorA);
            j["colorB"] = ToJson(c.colorB);
            j["endColor"] = ToJson(c.endColor);
            j["intensity"] = c.intensity;
            j["softness"] = c.softness;
            j["stretch"] = c.stretch;
            j["speedBrightness"] = c.speedBrightness;
            // 連鎖
            j["followUps"] = json::array();
            for (const ParticleFollowUp& followUp : c.followUps) {
                json f;
                f["effect"] = followUp.effectName;
                f["delay"] = followUp.delay;
                f["samePosition"] = followUp.samePosition;
                j["followUps"].push_back(f);
            }
            return j;
        }

        // 型が合わない値があると json::exception が投げられる (呼び出し側で catch する)
        ParticleConfig ConfigFromJson(const json& j) {
            using N = ParticleNames;
            const ParticleConfig d{};
            ParticleConfig c{};
            // 発生
            c.emitMode = ReadEnum(j, "emitMode", d.emitMode, N::kEmitModeIds);
            c.maxParticles = j.value("maxParticles", d.maxParticles);
            c.burstCount = j.value("burstCount", d.burstCount);
            c.burstRepeat = j.value("burstRepeat", d.burstRepeat);
            c.burstInterval = j.value("burstInterval", d.burstInterval);
            c.rate = j.value("rate", d.rate);
            c.duration = j.value("duration", d.duration);
            c.useUnscaledTime = j.value("useUnscaledTime", d.useUnscaledTime);
            // 位置・連携
            c.positionKey = j.value("positionKey", d.positionKey);
            c.basePosition = ReadVector3(j, "basePosition", d.basePosition);
            c.positionOffset = ReadVector3(j, "positionOffset", d.positionOffset);
            c.targetKey = j.value("targetKey", d.targetKey);
            c.targetOffset = ReadVector3(j, "targetOffset", d.targetOffset);
            c.triggerFlagKey = j.value("triggerFlagKey", d.triggerFlagKey);
            c.stopWhenFlagOff = j.value("stopWhenFlagOff", d.stopWhenFlagOff);
            // 形
            c.spawnShape = ReadEnum(j, "spawnShape", d.spawnShape, N::kSpawnShapeIds);
            c.radius = j.value("radius", d.radius);
            c.innerRatio = j.value("innerRatio", d.innerRatio);
            c.height = j.value("height", d.height);
            c.boxSize = ReadVector3(j, "boxSize", d.boxSize);
            c.lineEnd = ReadVector3(j, "lineEnd", d.lineEnd);
            c.shapeAxis = ReadVector3(j, "shapeAxis", d.shapeAxis);
            c.shrinkTime = j.value("shrinkTime", d.shrinkTime);
            c.shrinkEndRatio = j.value("shrinkEndRatio", d.shrinkEndRatio);
            // 初速
            c.velocityMode = ReadEnum(j, "velocityMode", d.velocityMode, N::kVelocityModeIds);
            c.velocity = ReadVector3(j, "velocity", d.velocity);
            c.velocityMin = ReadVector3(j, "velocityMin", d.velocityMin);
            c.velocityMax = ReadVector3(j, "velocityMax", d.velocityMax);
            c.speedMin = j.value("speedMin", d.speedMin);
            c.speedMax = j.value("speedMax", d.speedMax);
            // 寿命・大きさ・回転
            c.lifeMin = j.value("lifeMin", d.lifeMin);
            c.lifeMax = j.value("lifeMax", d.lifeMax);
            c.scaleMin = j.value("scaleMin", d.scaleMin);
            c.scaleMax = j.value("scaleMax", d.scaleMax);
            c.startScale = j.value("startScale", d.startScale);
            c.endScale = j.value("endScale", d.endScale);
            c.randomRotation = j.value("randomRotation", d.randomRotation);
            c.rotationSpeedMin = j.value("rotationSpeedMin", d.rotationSpeedMin);
            c.rotationSpeedMax = j.value("rotationSpeedMax", d.rotationSpeedMax);
            // 動き方
            c.motion = ReadEnum(j, "motion", d.motion, N::kMotionIds);
            c.gravity = j.value("gravity", d.gravity);
            c.drag = j.value("drag", d.drag);
            c.noiseStrength = j.value("noiseStrength", d.noiseStrength);
            c.noiseFrequency = j.value("noiseFrequency", d.noiseFrequency);
            c.motionAxis = ReadVector3(j, "motionAxis", d.motionAxis);
            c.axisFacesCamera = j.value("axisFacesCamera", d.axisFacesCamera);
            c.arcAngle = j.value("arcAngle", d.arcAngle);
            c.arcBothDirections = j.value("arcBothDirections", d.arcBothDirections);
            c.easePower = j.value("easePower", d.easePower);
            c.homingSpeed = j.value("homingSpeed", d.homingSpeed);
            c.homingTurn = j.value("homingTurn", d.homingTurn);
            c.arriveRadius = j.value("arriveRadius", d.arriveRadius);
            c.vortexAngularSpeed = j.value("vortexAngularSpeed", d.vortexAngularSpeed);
            c.vortexRadialSpeed = j.value("vortexRadialSpeed", d.vortexRadialSpeed);
            c.vortexRiseSpeed = j.value("vortexRiseSpeed", d.vortexRiseSpeed);
            c.controlOffsetMin = ReadVector3(j, "controlOffsetMin", d.controlOffsetMin);
            c.controlOffsetMax = ReadVector3(j, "controlOffsetMax", d.controlOffsetMax);
            c.bounce = j.value("bounce", d.bounce);
            c.floorY = j.value("floorY", d.floorY);
            c.restitution = j.value("restitution", d.restitution);
            c.friction = j.value("friction", d.friction);
            c.jitterStrength = j.value("jitterStrength", d.jitterStrength);
            c.jitterFrequency = j.value("jitterFrequency", d.jitterFrequency);
            // 消え方
            c.death = ReadEnum(j, "death", d.death, N::kDeathIds);
            c.deathRatio = j.value("deathRatio", d.deathRatio);
            c.blinkFrequencyMin = j.value("blinkFrequencyMin", d.blinkFrequencyMin);
            c.blinkFrequencyMax = j.value("blinkFrequencyMax", d.blinkFrequencyMax);
            c.blinkFade = j.value("blinkFade", d.blinkFade);
            c.deathStretch = j.value("deathStretch", d.deathStretch);
            c.deathColor = ReadVector4(j, "deathColor", d.deathColor);
            c.splitCount = j.value("splitCount", d.splitCount);
            c.splitSpeed = j.value("splitSpeed", d.splitSpeed);
            c.splitLife = j.value("splitLife", d.splitLife);
            c.splitScale = j.value("splitScale", d.splitScale);
            c.absorbStrength = j.value("absorbStrength", d.absorbStrength);
            // 見た目
            c.look = ReadEnum(j, "look", d.look, N::kLookIds);
            c.texturePath = j.value("texturePath", d.texturePath);
            c.blend = ReadEnum(j, "blend", d.blend, N::kBlendIds);
            c.colorA = ReadVector4(j, "colorA", d.colorA);
            c.colorB = ReadVector4(j, "colorB", d.colorB);
            c.endColor = ReadVector4(j, "endColor", d.endColor);
            c.intensity = j.value("intensity", d.intensity);
            c.softness = j.value("softness", d.softness);
            c.stretch = j.value("stretch", d.stretch);
            c.speedBrightness = j.value("speedBrightness", d.speedBrightness);
            // 連鎖
            auto followUps = j.find("followUps");
            if (followUps != j.end() && followUps->is_array()) {
                const ParticleFollowUp defaultFollowUp{};
                for (const json& f : *followUps) {
                    if (!f.is_object()) {
                        continue;
                    }
                    ParticleFollowUp followUp{};
                    followUp.effectName = f.value("effect", defaultFollowUp.effectName);
                    followUp.delay = f.value("delay", defaultFollowUp.delay);
                    followUp.samePosition = f.value("samePosition", defaultFollowUp.samePosition);
                    c.followUps.push_back(followUp);
                }
            }
            return c;
        }

#ifdef NRELEASE
        // --- ImGui ---
        template <typename E, size_t N>
        bool ComboEnum(const char* label, E& value, const char* const (&labels)[N]) {
            int index = static_cast<int>(value);
            if (ImGui::Combo(label, &index, labels, static_cast<int>(N))) {
                value = static_cast<E>(index);
                return true;
            }
            return false;
        }

        bool DragU32(const char* label, uint32_t& value, float speed, int minValue, int maxValue) {
            int temp = static_cast<int>(value);
            if (ImGui::DragInt(label, &temp, speed, minValue, maxValue)) {
                value = static_cast<uint32_t>(std::clamp(temp, minValue, maxValue));
                return true;
            }
            return false;
        }

        // 最小・最大の2つを1行で編集する
        void DragRange(const char* label, float& minValue, float& maxValue, float speed, float lower, float upper) {
            float values[2] = { minValue, maxValue };
            if (ImGui::DragFloat2(label, values, speed, lower, upper)) {
                minValue = values[0];
                maxValue = values[1];
            }
        }
#endif
    }

    // =====================================================================
    // 初期化・終了
    // =====================================================================

    ParticleManager* ParticleManager::GetInstance() {
        static ParticleManager instance;
        return &instance;
    }

    void ParticleManager::Initialize() {
        Logger::Log("ParticleManager : Initializing...\n");
        // NOTE: エフェクトの読み込みは、アプリが SetFolderPath してから LoadOrCreateDefaults() で行う
        Logger::LogSuccess("ParticleManager : Initialized\n");
    }

    bool ParticleManager::LoadOrCreateDefaults() {
        // 保存したファイルが無ければ、テンプレから見本を作る
        if (Load()) {
            return true;
        }
        CreateDefaultEffects();
        return false;
    }

    void ParticleManager::Finalize() {
        ParticleManager* instance = GetInstance();

        for (auto& [name, effect] : instance->effects_) {
            effect->emitter.Finalize();
        }
        instance->effects_.clear();
    }

    // =====================================================================
    // 作成・削除
    // =====================================================================

    uint32_t ParticleManager::SumMaxParticlesExcept(const Effect* exclude) const {
        uint32_t total = 0;
        for (const auto& [name, effect] : effects_) {
            if (effect && effect.get() != exclude) {
                total += effect->config.maxParticles;
            }
        }
        return total;
    }

    void ParticleManager::ApplyConfig(Effect& effect, bool forceRecreate, uint32_t othersMaxTotal) {
        ParticleConfig& c = effect.config;
        if (c.maxParticles < 1) {
            c.maxParticles = 1;
        }

        // 全体の予算を超える分は切り詰める
        const uint32_t remaining = (othersMaxTotal < particleBudget_) ? (particleBudget_ - othersMaxTotal) : 0;
        if (c.maxParticles > remaining) {
            const uint32_t requested = c.maxParticles;
            c.maxParticles = (std::max)(remaining, 1u);
            Logger::LogWarning("[ParticleManager] 全体の予算({})を超えるので、最大数を {} → {} に減らしました\n",
                particleBudget_, requested, c.maxParticles);
        }

        // 最大数・分裂の有無が変わったときだけGPUバッファを作り直す (出ている粒は引き継ぐ)
        const bool useSplit = (c.death == ParticleDeath::Split);
        if (forceRecreate || !effect.emitter.IsInitialized()) {
            effect.emitter.Initialize(c.maxParticles, useSplit);
        } else if (effect.emitter.GetMaxCount() != c.maxParticles || effect.emitter.UsesSplit() != useSplit) {
            effect.emitter.Resize(c.maxParticles, useSplit);
        }

        if (!effect.textureLoaded || effect.loadedTexturePath != c.texturePath) {
            effect.textureHandle = c.texturePath.empty() ? TextureManager::GetWhiteTex() : LoadTex(c.texturePath);
            effect.loadedTexturePath = c.texturePath;
            effect.textureLoaded = true;
        }
    }

    bool ParticleManager::Create(const std::string& name, const ParticleConfig& config) {
        ParticleManager* instance = GetInstance();

        if (name.empty()) {
            Logger::LogWarning("[ParticleManager] Create: 名前が空です\n");
            return false;
        }
        if (instance->effects_.contains(name)) {
            Logger::LogWarning("[ParticleManager] Create: 同じ名前のエフェクトがあります: {}\n", name);
            return false;
        }

        auto effect = std::make_unique<Effect>();
        effect->config = config;
        instance->ApplyConfig(*effect, true, instance->SumMaxParticlesExcept(nullptr));
        instance->effects_[name] = std::move(effect);
        return true;
    }

    void ParticleManager::Destroy(const std::string& name) {
        ParticleManager* instance = GetInstance();

        auto it = instance->effects_.find(name);
        if (it == instance->effects_.end()) {
            return;
        }
        it->second->emitter.Finalize();
        instance->effects_.erase(it);
    }

    bool ParticleManager::Contains(const std::string& name) {
        return GetInstance()->effects_.contains(name);
    }

    std::vector<std::string> ParticleManager::GetNames() {
        std::vector<std::string> names;
        for (const auto& [name, effect] : GetInstance()->effects_) {
            names.push_back(name);
        }
        return names;
    }

    ParticleConfig* ParticleManager::GetConfig(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        return (it != instance->effects_.end()) ? &it->second->config : nullptr;
    }

    void ParticleManager::SetMaxParticles(const std::string& name, uint32_t maxParticles) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        if (it == instance->effects_.end()) {
            Logger::LogWarning("[ParticleManager] SetMaxParticles: エフェクトがありません: {}\n", name);
            return;
        }
        Effect& effect = *it->second;
        effect.config.maxParticles = maxParticles;
        instance->ApplyConfig(effect, false, instance->SumMaxParticlesExcept(&effect));
    }

    void ParticleManager::CreateDefaultEffects() {
        for (const ParticleTemplates::Entry& entry : ParticleTemplates::GetAll()) {
            if (entry.make == &ParticleTemplates::Blank || Contains(entry.defaultName)) {
                continue;
            }
            Create(entry.defaultName, entry.make());
        }
    }

    // =====================================================================
    // 再生
    // =====================================================================

    void ParticleManager::Play(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        if (it == instance->effects_.end()) {
            Logger::LogWarning("[ParticleManager] Play: エフェクトがありません: {}\n", name);
            return;
        }
        instance->PlayImpl(*it->second, nullptr);
    }

    void ParticleManager::Play(const std::string& name, const Vector3& position) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        if (it == instance->effects_.end()) {
            Logger::LogWarning("[ParticleManager] Play: エフェクトがありません: {}\n", name);
            return;
        }
        instance->PlayImpl(*it->second, &position);
    }

    void ParticleManager::Stop(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        if (it != instance->effects_.end()) {
            it->second->playing = false;
        }
    }

    void ParticleManager::Clear(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        if (it != instance->effects_.end()) {
            it->second->playing = false;
            it->second->emitter.RequestClear();
        }
    }

    void ParticleManager::StopAll() {
        ParticleManager* instance = GetInstance();
        for (auto& [name, effect] : instance->effects_) {
            effect->playing = false;
        }
        instance->pendingPlays_.clear();
    }

    void ParticleManager::ClearAll() {
        ParticleManager* instance = GetInstance();
        for (auto& [name, effect] : instance->effects_) {
            effect->playing = false;
            effect->emitter.RequestClear();
        }
        instance->pendingPlays_.clear();
    }

    bool ParticleManager::IsPlaying(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        return (it != instance->effects_.end()) && it->second->playing;
    }

    uint32_t ParticleManager::GetAliveCount(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        return (it != instance->effects_.end()) ? it->second->emitter.GetAliveCount() : 0;
    }

    uint32_t ParticleManager::GetMaxCount(const std::string& name) {
        ParticleManager* instance = GetInstance();
        auto it = instance->effects_.find(name);
        return (it != instance->effects_.end()) ? it->second->emitter.GetMaxCount() : 0;
    }

    uint32_t ParticleManager::GetTotalAliveCount() {
        uint32_t total = 0;
        for (const auto& [name, effect] : GetInstance()->effects_) {
            total += effect->emitter.GetAliveCount();
        }
        return total;
    }

    uint32_t ParticleManager::GetTotalMaxCount() {
        uint32_t total = 0;
        for (const auto& [name, effect] : GetInstance()->effects_) {
            total += effect->emitter.GetMaxCount();
        }
        return total;
    }

    void ParticleManager::PlayImpl(Effect& effect, const Vector3* position) {
        const ParticleConfig& c = effect.config;

        effect.playing = true;
        effect.elapsed = 0.0f;
        effect.emitAccumulator = 0.0f;
        effect.burstsDone = 0;
        effect.burstTimer = 0.0f;
        effect.hasPlayPosition = (position != nullptr);
        if (position) {
            effect.playPosition = *position;
        }

        // 連鎖：「何秒後に何を再生するか」を予約しておく (時間は UpdatePendingPlays で進める)
        if (!c.followUps.empty()) {
            const Vector3 origin = ResolveOrigin(effect);
            for (const ParticleFollowUp& followUp : c.followUps) {
                if (followUp.effectName.empty()) {
                    continue;
                }
                if (pendingPlays_.size() >= maxPendingPlays_) {
                    Logger::LogWarning("[ParticleManager] 連鎖の予約が多すぎるので無視しました (循環していませんか？): {}\n", followUp.effectName);
                    break;
                }
                PendingPlay pending{};
                pending.effectName = followUp.effectName;
                pending.remaining = (std::max)(followUp.delay, 0.0f);
                pending.hasPosition = followUp.samePosition;
                pending.position = origin;
                pending.unscaledTime = c.useUnscaledTime;
                pendingPlays_.push_back(pending);
            }
        }

        // まとめて出すものは、呼んだその場で1回目を出す
        // (同じフレームに場所を変えて何回 Play しても、それぞれの場所に出る)
        if (c.emitMode == ParticleEmitMode::Burst) {
            EmitParticles(effect, c.burstCount);
            effect.burstsDone = 1;
            effect.burstTimer = c.burstInterval;
            if (effect.burstsDone >= c.burstRepeat) {
                effect.playing = false;
            }
        }
    }

    // =====================================================================
    // 位置・軸の解決
    // =====================================================================

    Vector3 ParticleManager::ResolveOrigin(const Effect& effect) const {
        const ParticleConfig& c = effect.config;
        // Play で場所を指定されていればそれが優先。次に追従キー、最後に基準座標
        if (effect.hasPlayPosition) {
            return effect.playPosition + c.positionOffset;
        }
        if (const Vector3* tracked = BindingRegistry::FindPosition(c.positionKey)) {
            return *tracked + c.positionOffset;
        }
        return c.basePosition + c.positionOffset;
    }

    Vector3 ParticleManager::ResolveTarget(const Effect& effect, const Vector3& origin) const {
        const ParticleConfig& c = effect.config;
        if (const Vector3* tracked = BindingRegistry::FindPosition(c.targetKey)) {
            return *tracked + c.targetOffset;
        }
        return origin + c.targetOffset;
    }

    bool ParticleManager::HasLiveTarget(const Effect& effect) const {
        // 目標が動くもの (キーで追従している) なら、毎フレームの目標を全粒で使う。
        // そうでなければ、粒ごとに発生時の目標を覚えておく (別々の場所で Play しても混ざらない)
        const ParticleConfig& c = effect.config;
        if (BindingRegistry::FindPosition(c.targetKey) != nullptr) {
            return true;
        }
        return !effect.hasPlayPosition && BindingRegistry::FindPosition(c.positionKey) != nullptr;
    }

    Vector3 ParticleManager::ResolveShapeAxis(const ParticleConfig& config) const {
        Vector3 axis = config.axisFacesCamera ? CameraManager::GetActive().GetForward() : config.shapeAxis;
        axis = Normalize(axis);
        if (Length(axis) < 0.5f) {
            axis = { 0.0f, 1.0f, 0.0f };
        }
        return axis;
    }

    Vector3 ParticleManager::ResolveMotionAxis(const ParticleConfig& config) const {
        Vector3 axis = config.axisFacesCamera ? CameraManager::GetActive().GetForward() : config.motionAxis;
        axis = Normalize(axis);
        if (Length(axis) < 0.5f) {
            axis = { 0.0f, 1.0f, 0.0f };
        }
        return axis;
    }

    float ParticleManager::GetDeltaTime(const ParticleConfig& config) const {
        const float dt = config.useUnscaledTime ? TimeManager::GetDeltaTime() : TimeManager::GetScaleTime();
        return std::clamp(dt, 0.0f, maxDeltaTime_);
    }

    // =====================================================================
    // 発生
    // =====================================================================

    void ParticleManager::EmitParticles(Effect& effect, uint32_t count) {
        if (count == 0 || !effect.emitter.IsInitialized()) {
            return;
        }
        const ParticleConfig& c = effect.config;

        // 1粒ずつの初期値はGPUが作るので、ここでは「どこに何粒」の命令を1つ作るだけ
        const Vector3 origin = ResolveOrigin(effect);
        const Vector3 target = ResolveTarget(effect, origin);

        // 輪・円柱の軸と、それに直交する2本
        const Vector3 axis = ResolveShapeAxis(c);
        const Vector3 helper = (std::fabs(axis.y) < 0.99f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
        const Vector3 tangent1 = Normalize(Cross(axis, helper));
        const Vector3 tangent2 = Cross(axis, tangent1);

        // 発生半径の縮み (再生してからの時間で縮む)
        float radiusScale = 1.0f;
        if (c.shrinkTime > 0.0f) {
            const float s = std::clamp(effect.elapsed / c.shrinkTime, 0.0f, 1.0f);
            radiusScale = 1.0f + (c.shrinkEndRatio - 1.0f) * s;
        }

        ParticleSpawnBatch batch{};
        batch.origin = origin;
        batch.count = count;
        batch.target = target;
        batch.firstIndex = 0;
        batch.axis = axis;
        batch.radiusScale = radiusScale;
        batch.tangent1 = tangent1;
        batch.seed = static_cast<uint32_t>(randomEngine_());
        batch.tangent2 = tangent2;
        batch.padding = 0.0f;

        if (!effect.emitter.AddBatch(batch)) {
            const uint32_t limit = ParticleEmitter::kMaxBatchesPerFrame;
            Logger::LogWarning("[ParticleManager] 1フレームに発生させられる回数({}回)を超えたので無視しました\n", limit);
        }
    }

    // =====================================================================
    // 更新・描画
    // =====================================================================

    void ParticleManager::Update() {
        ParticleManager* instance = GetInstance();

        // ホットリロード (スローの影響を受けない時間で見張る)
        const float unscaledDeltaTime = TimeManager::GetDeltaTime();
        ParticleCommon::CheckHotReload(unscaledDeltaTime);
        instance->CheckJsonHotReload(unscaledDeltaTime);

        // 連鎖で予約された再生 (各エフェクトの発生より先に行う)
        instance->UpdatePendingPlays();

        for (auto& [name, effect] : instance->effects_) {
            instance->UpdateEffect(*effect);
        }
    }

    void ParticleManager::UpdatePendingPlays() {
        if (pendingPlays_.empty()) {
            return;
        }
        const float scaledDeltaTime = std::clamp(TimeManager::GetScaleTime(), 0.0f, maxDeltaTime_);
        const float unscaledDeltaTime = std::clamp(TimeManager::GetDeltaTime(), 0.0f, maxDeltaTime_);

        // 時間が来たものを取り出してから再生する (再生した先でさらに予約が増えても大丈夫なように)
        std::vector<PendingPlay> ready;
        for (auto it = pendingPlays_.begin(); it != pendingPlays_.end();) {
            it->remaining -= it->unscaledTime ? unscaledDeltaTime : scaledDeltaTime;
            if (it->remaining <= 0.0f) {
                ready.push_back(*it);
                it = pendingPlays_.erase(it);
            } else {
                ++it;
            }
        }

        for (const PendingPlay& pending : ready) {
            auto found = effects_.find(pending.effectName);
            if (found == effects_.end()) {
                Logger::LogWarning("[ParticleManager] 連鎖: エフェクトがありません: {}\n", pending.effectName);
                continue;
            }
            PlayImpl(*found->second, pending.hasPosition ? &pending.position : nullptr);
        }
    }

    void ParticleManager::UpdateEffect(Effect& effect) {
        const ParticleConfig& c = effect.config;
        const float dt = GetDeltaTime(c);
        effect.time += dt;

        // --- フラグで再生 ---
        const bool* flag = BindingRegistry::FindFlag(c.triggerFlagKey);
        if (flag != effect.lastFlagPtr) {
            // 参照先が変わったフレームは、偽の「立ち上がり」を拾わないよう今の値に合わせるだけ
            effect.lastFlagPtr = flag;
            effect.previousFlag = flag ? *flag : false;
        } else if (flag) {
            const bool current = *flag;
            if (current && !effect.previousFlag) {
                PlayImpl(effect, nullptr);
            } else if (!current && effect.previousFlag && c.stopWhenFlagOff && c.emitMode == ParticleEmitMode::Continuous) {
                effect.playing = false;
            }
            effect.previousFlag = current;
        }

        // --- 発生 ---
        if (effect.playing) {
            effect.elapsed += dt;
            if (c.emitMode == ParticleEmitMode::Burst) {
                if (effect.burstsDone < c.burstRepeat) {
                    effect.burstTimer -= dt;
                    if (effect.burstTimer <= 0.0f) {
                        EmitParticles(effect, c.burstCount);
                        ++effect.burstsDone;
                        effect.burstTimer += c.burstInterval;
                    }
                }
                if (effect.burstsDone >= c.burstRepeat) {
                    effect.playing = false;
                }
            } else {
                effect.emitAccumulator += (std::max)(c.rate, 0.0f) * dt;
                const uint32_t emitCount = static_cast<uint32_t>(effect.emitAccumulator);
                effect.emitAccumulator -= static_cast<float>(emitCount);
                EmitParticles(effect, emitCount);
                if (c.duration > 0.0f && effect.elapsed >= c.duration) {
                    effect.playing = false;
                }
            }
        }

        // --- シミュレーションのパラメータ ---
        const Vector3 origin = ResolveOrigin(effect);

        ParticleSimParams params{};
        params.deltaTime = dt;
        params.motion = static_cast<uint32_t>(c.motion);
        params.gravity = c.gravity;
        params.drag = (std::max)(c.drag, 0.0f);
        params.noiseStrength = c.noiseStrength;
        params.noiseFrequency = c.noiseFrequency;
        params.liveTarget = ResolveTarget(effect, origin);
        params.useLiveTarget = HasLiveTarget(effect) ? 1u : 0u;
        params.axis = ResolveMotionAxis(c);
        params.arcAngle = DegToRad(c.arcAngle);
        params.easePower = (std::max)(c.easePower, 0.01f);
        params.homingSpeed = c.homingSpeed;
        params.homingTurn = c.homingTurn;
        params.arriveRadius = c.arriveRadius;
        params.vortexAngularSpeed = DegToRad(c.vortexAngularSpeed);
        params.vortexRadialSpeed = c.vortexRadialSpeed;
        params.vortexRiseSpeed = c.vortexRiseSpeed;
        params.arcBothDirections = c.arcBothDirections ? 1u : 0u;
        params.bounce = c.bounce ? 1u : 0u;
        params.floorY = c.floorY;
        params.restitution = c.restitution;
        params.friction = c.friction;
        params.death = static_cast<uint32_t>(c.death);
        params.deathRatio = std::clamp(c.deathRatio, 0.0f, 1.0f);
        params.absorbStrength = c.absorbStrength;
        params.time = effect.time;
        params.splitCount = c.splitCount;
        params.splitSpeed = c.splitSpeed;
        params.splitLife = (std::max)(c.splitLife, 0.01f);
        params.splitScale = c.splitScale;

        // --- 1粒の初期値をGPUで作るためのパラメータ ---
        ParticleSpawnParams spawn{};
        spawn.shape = static_cast<uint32_t>(c.spawnShape);
        spawn.radius = c.radius;
        spawn.innerRatio = std::clamp(c.innerRatio, 0.0f, 1.0f);
        spawn.height = c.height;
        spawn.boxSize = c.boxSize;
        spawn.velocityMode = static_cast<uint32_t>(c.velocityMode);
        spawn.lineEnd = c.lineEnd;
        spawn.randomRotation = c.randomRotation ? 1u : 0u;
        spawn.velocity = c.velocity;
        spawn.speedMin = c.speedMin;
        spawn.velocityMin = c.velocityMin;
        spawn.speedMax = c.speedMax;
        spawn.velocityMax = c.velocityMax;
        spawn.lifeMin = c.lifeMin;
        spawn.controlOffsetMin = c.controlOffsetMin;
        spawn.lifeMax = c.lifeMax;
        spawn.controlOffsetMax = c.controlOffsetMax;
        spawn.scaleMin = c.scaleMin;
        spawn.colorA = c.colorA;
        spawn.colorB = c.colorB;
        spawn.scaleMax = c.scaleMax;
        spawn.rotationSpeedMin = DegToRad(c.rotationSpeedMin);
        spawn.rotationSpeedMax = DegToRad(c.rotationSpeedMax);
        spawn.blinkFrequencyMin = c.blinkFrequencyMin;
        spawn.blinkFrequencyMax = c.blinkFrequencyMax;

        effect.emitter.Update(params, spawn);
    }

    void ParticleManager::Draw() {
        ParticleManager* instance = GetInstance();

        // CameraManager::UpdateAll() で確定済みの描画カメラを使う
        const Camera& camera = CameraManager::GetActive();
        const Matrix4x4 cameraWorld = Inverse(camera.GetViewMatrix());
        const Vector3 cameraRight = Normalize(Vector3{ cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] });
        const Vector3 cameraUp = Normalize(Vector3{ cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] });

        for (auto& [name, effect] : instance->effects_) {
            const ParticleConfig& c = effect->config;

            ParticleRenderParams params{};
            params.viewProjection = camera.GetViewProjectionMatrix();
            params.cameraRight = cameraRight;
            params.time = effect->time;
            params.cameraUp = cameraUp;
            params.death = static_cast<uint32_t>(c.death);
            params.endColor = c.endColor;
            params.deathColor = c.deathColor;
            params.startScale = c.startScale;
            params.endScale = c.endScale;
            params.deathRatio = std::clamp(c.deathRatio, 0.0f, 1.0f);
            params.look = static_cast<uint32_t>(c.look);
            params.stretch = c.stretch;
            params.speedBrightness = c.speedBrightness;
            params.jitterStrength = c.jitterStrength;
            params.jitterFrequency = c.jitterFrequency;
            params.blinkFade = std::clamp(c.blinkFade, 0.0f, 1.0f);
            params.intensity = c.intensity;
            params.deathStretch = c.deathStretch;
            params.softness = c.softness;

            effect->emitter.Draw(params, effect->textureHandle, c.blend);
        }
    }

    // =====================================================================
    // 保存・読込
    // =====================================================================

    void ParticleManager::SetFolderPath(const std::string& path) {
        ParticleManager* instance = GetInstance();
        instance->folderPath_ = path;
        if (!instance->folderPath_.empty() && instance->folderPath_.back() != '/') {
            instance->folderPath_ += '/';
        }
    }

    void ParticleManager::Save() {
        ParticleManager* instance = GetInstance();

        const std::string fullPath = instance->GetFullPath();
#ifdef NRELEASE
        // 既にファイルがあれば、エディタで上書きしてよいか確認してから保存する
        if (std::filesystem::exists(fullPath)) {
            instance->showOverwriteModal_ = true;
        } else {
            instance->SaveToFile(fullPath);
        }
#else
        instance->SaveToFile(fullPath);
#endif
    }

    void ParticleManager::SaveToFile(const std::string& fullPath) {
        std::error_code ec;
        std::filesystem::create_directories(folderPath_, ec);

        std::ofstream file(fullPath);
        if (!file.is_open()) {
            Logger::LogError("[ParticleManager] Save: ファイルを開けませんでした: {}\n", fullPath);
            return;
        }

        json root;
        root["version"] = 1;
        root["effects"] = json::object();
        for (const auto& [name, effect] : effects_) {
            root["effects"][name] = ConfigToJson(effect->config);
        }
        file << root.dump(4, ' ', false, json::error_handler_t::replace) << "\n";
        file.close();

        // 自分で保存した分は、ホットリロードで読み直さない
        RememberJsonWriteTime();
        Logger::LogSuccess("[ParticleManager] Save: 保存しました: {}\n", fullPath);
    }

    bool ParticleManager::Load() {
        ParticleManager* instance = GetInstance();
        const std::string fullPath = instance->GetFullPath();

        std::ifstream file(fullPath);
        if (!file.is_open()) {
            Logger::LogWarning("[ParticleManager] Load: ファイルがありません: {}\n", fullPath);
            instance->jsonExists_ = false;
            return false;
        }
        // 読み込みに失敗しても、同じ内容で何度も読み直さないよう日時は覚えておく
        instance->RememberJsonWriteTime();

        json root = json::parse(file, nullptr, false); // false: 失敗時に例外ではなく discarded を返す
        if (root.is_discarded() || !root.is_object() || !root.contains("effects") || !root["effects"].is_object()) {
            Logger::LogError("[ParticleManager] Load: JSONの形式が正しくありません: {}\n", fullPath);
            return false;
        }

        // 今の状態を壊す前に、先に全部読んでおく
        std::vector<std::pair<std::string, ParticleConfig>> loaded;
        try {
            const json& effectsJson = root["effects"];
            for (auto it = effectsJson.begin(); it != effectsJson.end(); ++it) {
                loaded.emplace_back(it.key(), ConfigFromJson(it.value()));
            }
        }
        catch (const json::exception& e) {
            const std::string message = e.what();
            Logger::LogError("[ParticleManager] Load: 読み込みに失敗しました: {}\n", message);
            return false;
        }

        // 同じ名前のエフェクトは作り直さずに設定だけ入れ替える (再生中の粒が消えないように)
        std::map<std::string, std::unique_ptr<Effect>> newEffects;
        uint32_t loadedMaxTotal = 0; // 予算の判定用 (読み込んだ順に足していく)
        for (auto& [name, config] : loaded) {
            std::unique_ptr<Effect> effect;
            auto it = instance->effects_.find(name);
            if (it != instance->effects_.end()) {
                effect = std::move(it->second);
            } else {
                effect = std::make_unique<Effect>();
            }
            effect->config = config;
            instance->ApplyConfig(*effect, false, loadedMaxTotal);
            loadedMaxTotal += effect->config.maxParticles;
            newEffects[name] = std::move(effect);
        }
        for (auto& [name, effect] : instance->effects_) {
            if (effect) {
                effect->emitter.Finalize();
            }
        }
        instance->effects_ = std::move(newEffects);

        Logger::LogSuccess("[ParticleManager] Load: 読み込みました: {}\n", fullPath);
        return true;
    }

    void ParticleManager::RememberJsonWriteTime() {
        std::error_code ec;
        const auto writeTime = std::filesystem::last_write_time(GetFullPath(), ec);
        if (ec) {
            jsonExists_ = false;
            return;
        }
        jsonWriteTime_ = writeTime;
        jsonExists_ = true;
    }

    void ParticleManager::CheckJsonHotReload(float deltaTime) {
#ifdef NRELEASE
        if (!autoReloadJson_) {
            return;
        }
        jsonWatchTimer_ -= deltaTime;
        if (jsonWatchTimer_ > 0.0f) {
            return;
        }
        jsonWatchTimer_ = jsonWatchInterval_;

        std::error_code ec;
        const auto writeTime = std::filesystem::last_write_time(GetFullPath(), ec);
        if (ec) {
            return;
        }
        if (!jsonExists_ || writeTime != jsonWriteTime_) {
            Logger::Log("[ParticleManager] JSONが更新されたので読み込み直します\n");
            Load();
        }
#else
        (void)deltaTime;
#endif
    }

    // =====================================================================
    // エディタ
    // =====================================================================

    void ParticleManager::DrawImGui() {
#ifdef NRELEASE
        ParticleManager* instance = GetInstance();
        const auto& templates = ParticleTemplates::GetAll();
        std::vector<const char*> templateLabels;
        for (const auto& entry : templates) {
            templateLabels.push_back(entry.label.c_str());
        }
        const int templateCount = static_cast<int>(templateLabels.size());

        ImGui::Begin("ParticleManager");

        // --- 保存・読込 ---
        const std::string fullPath = instance->GetFullPath();
        ImGui::Text("保存先 : %s", fullPath.c_str());
        if (ImGui::Button("Save")) {
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            Load();
        }
        ImGui::SameLine();
        ImGui::Checkbox("JSONが更新されたら自動で読み込み", &instance->autoReloadJson_);
        if (ImGui::Button("見本のエフェクトを追加 (無いものだけ)")) {
            CreateDefaultEffects();
        }
        ImGui::SameLine();
        if (ImGui::Button("全部止める")) {
            StopAll();
        }
        ImGui::SameLine();
        if (ImGui::Button("全部消す")) {
            ClearAll();
        }

        ImGui::Separator();
        ParticleCommon::DrawImGui();
        ImGui::Separator();

        // --- 新規作成 ---
        if (ImGui::CollapsingHeader("新規作成")) {
            ImGui::InputText("名前##new", instance->newNameBuffer_, sizeof(instance->newNameBuffer_));
            instance->newTemplateIndex_ = std::clamp(instance->newTemplateIndex_, 0, templateCount - 1);
            ImGui::Combo("テンプレ##new", &instance->newTemplateIndex_, templateLabels.data(), templateCount);
            if (ImGui::Button("作成")) {
                const std::string newName = instance->newNameBuffer_;
                const auto& entry = templates[static_cast<size_t>(instance->newTemplateIndex_)];
                if (Create(newName, entry.make())) {
                    instance->selectedName_ = newName;
                    instance->newNameBuffer_[0] = '\0';
                }
            }
        }

        // --- 一覧 (全体の出現数と、エフェクトごとの出現数) ---
        ImGui::Text("エフェクト一覧 (%d)", static_cast<int>(instance->effects_.size()));
        const uint32_t reservedTotal = GetTotalMaxCount();
        const bool overBudget = reservedTotal >= instance->particleBudget_;
        ImGui::TextColored(overBudget ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "確保 (最大数の合計) : %u / %u", reservedTotal, instance->particleBudget_);
        ImGui::Text("合計 (全エフェクト)");
        instance->DrawUsageGauge(GetTotalAliveCount(), GetTotalMaxCount(), -FLT_MIN);
        ImGui::BeginChild("ParticleList", ImVec2(0.0f, instance->listHeight_), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY);
        for (auto& [name, effect] : instance->effects_) {
            const bool selected = (name == instance->selectedName_);
            if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_None, ImVec2(instance->listNameWidth_, 0.0f))) {
                instance->selectedName_ = name;
            }
            ImGui::SameLine();
            ImGui::PushID(name.c_str());
            instance->DrawUsageGauge(effect->emitter.GetAliveCount(), effect->emitter.GetMaxCount(), -FLT_MIN);
            ImGui::PopID();
        }
        ImGui::EndChild();

        // --- 選んでいるエフェクトの編集 ---
        auto it = instance->effects_.find(instance->selectedName_);
        if (it != instance->effects_.end()) {
            instance->DrawEffectEditor(it->first, *it->second);
        } else {
            ImGui::TextDisabled("一覧からエフェクトを選んでください");
        }

        // --- 上書き確認ポップアップ ---
        if (instance->showOverwriteModal_) {
            ImGui::OpenPopup("ParticleManager 上書き確認");
            Logger::LogWarning("[ParticleManager] Overwrite check.");
            instance->showOverwriteModal_ = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("ParticleManager 上書き確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("該当フォルダには既に同名のファイルが存在します。上書きしますか？");
            ImGui::Text("保存先 : %s", fullPath.c_str());
            ImGui::Separator();

            if (ImGui::Button("上書きする", ImVec2(120, 0))) {
                instance->SaveToFile(fullPath);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
                Logger::Log("[ParticleManager] Save cancelled.");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
#endif
    }

#ifdef NRELEASE
    void ParticleManager::DrawUsageGauge(uint32_t alive, uint32_t capacity, float width) const {
        const float ratio = (capacity > 0)
            ? std::clamp(static_cast<float>(alive) / static_cast<float>(capacity), 0.0f, 1.0f)
            : 0.0f;

        // 緑 → (gaugeWarningRatio_ で) 黄 → (gaugeDangerRatio_ で) 赤 になめらかに変える
        Vector4 color{};
        if (ratio <= gaugeWarningRatio_) {
            const float t = (gaugeWarningRatio_ > 0.0f) ? ratio / gaugeWarningRatio_ : 1.0f;
            color = gaugeSafeColor_ + (gaugeWarningColor_ - gaugeSafeColor_) * t;
        } else {
            const float range = gaugeDangerRatio_ - gaugeWarningRatio_;
            const float t = (range > 0.0f) ? std::clamp((ratio - gaugeWarningRatio_) / range, 0.0f, 1.0f) : 1.0f;
            color = gaugeWarningColor_ + (gaugeDangerColor_ - gaugeWarningColor_) * t;
        }

        // 幅の指定が無ければ、残りの幅の gaugeWidthRatio_ 倍にする
        if (width < 0.0f) {
            width = (std::max)(ImGui::GetContentRegionAvail().x * gaugeWidthRatio_, 1.0f);
        }

        char overlay[64] = {};
        std::snprintf(overlay, sizeof(overlay), "%.2f%%  (%u / %u)", static_cast<double>(ratio * 100.0f), alive, capacity);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.x, color.y, color.z, color.w));
        ImGui::ProgressBar(ratio, ImVec2(width, 0.0f), overlay);
        ImGui::PopStyleColor();
    }

    void ParticleManager::DrawEffectEditor(const std::string& name, Effect& effect) {
        using N = ParticleNames;
        ParticleConfig& c = effect.config;
        bool needApply = false;
        bool requestDestroy = false;
        bool requestDuplicate = false;

        ImGui::PushID(name.c_str());
        ImGui::Separator();
        ImGui::Text("編集中 : %s", name.c_str());
        DrawUsageGauge(effect.emitter.GetAliveCount(), effect.emitter.GetMaxCount(), -FLT_MIN);

        // --- 操作 ---
        if (ImGui::Button("再生")) {
            PlayImpl(effect, nullptr);
        }
        ImGui::SameLine();
        if (ImGui::Button("停止")) {
            effect.playing = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("クリア")) {
            effect.playing = false;
            effect.emitter.RequestClear();
        }
        ImGui::SameLine();
        if (ImGui::Button("複製")) {
            requestDuplicate = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("削除")) {
            requestDestroy = true;
        }

        // --- テンプレを適用 (位置・連携の設定は残す) ---
        {
            const auto& templates = ParticleTemplates::GetAll();
            std::vector<const char*> labels;
            for (const auto& entry : templates) {
                labels.push_back(entry.label.c_str());
            }
            const int count = static_cast<int>(labels.size());
            applyTemplateIndex_ = std::clamp(applyTemplateIndex_, 0, count - 1);
            ImGui::Combo("##applyTemplate", &applyTemplateIndex_, labels.data(), count);
            ImGui::SameLine();
            if (ImGui::Button("テンプレを適用")) {
                ParticleConfig applied = templates[static_cast<size_t>(applyTemplateIndex_)].make();
                applied.positionKey = c.positionKey;
                applied.basePosition = c.basePosition;
                applied.targetKey = c.targetKey;
                applied.triggerFlagKey = c.triggerFlagKey;
                applied.stopWhenFlagOff = c.stopWhenFlagOff;
                c = applied;
                needApply = true;
            }
        }

        // --- 発生 ---
        if (ImGui::CollapsingHeader("発生", ImGuiTreeNodeFlags_DefaultOpen)) {
            ComboEnum("発生のさせ方", c.emitMode, N::kEmitModeLabels);
            // 最大数：ドラッグ中は数字だけ動かし、離したときにバッファを作り直す (出ている粒は引き継ぐ)
            const uint32_t othersTotal = SumMaxParticlesExcept(&effect);
            const uint32_t usable = (othersTotal < particleBudget_) ? (particleBudget_ - othersTotal) : 0;
            const int usableMax = static_cast<int>((std::min)(usable, static_cast<uint32_t>(INT32_MAX)));
            int maxParticles = (effect.editingMaxParticles >= 0) ? effect.editingMaxParticles : static_cast<int>(c.maxParticles);
            if (ImGui::DragInt("最大数", &maxParticles, maxParticlesDragSpeed_, 1, (std::max)(usableMax, 1))) {
                effect.editingMaxParticles = maxParticles;
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                c.maxParticles = static_cast<uint32_t>((std::max)(maxParticles, 1));
                effect.editingMaxParticles = -1;
                needApply = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(予算内で最大 %u)", usable);
            if (c.emitMode == ParticleEmitMode::Burst) {
                DragU32("1回の数", c.burstCount, 1.0f, 0, 100000);
                DragU32("回数", c.burstRepeat, 0.1f, 1, 100);
                ImGui::DragFloat("間隔(秒)", &c.burstInterval, 0.01f, 0.0f, 10.0f);
            } else {
                ImGui::DragFloat("1秒あたりの数", &c.rate, 1.0f, 0.0f, 100000.0f);
                ImGui::DragFloat("出し続ける時間(秒) 0で止めるまで", &c.duration, 0.01f, 0.0f, 60.0f);
            }
            ImGui::Checkbox("スローの影響を受けない", &c.useUnscaledTime);
        }

        // --- 位置・連携 ---
        if (ImGui::CollapsingHeader("位置・連携")) {
            BindingRegistry::DrawPositionCombo("発生位置を追従", c.positionKey);
            ImGui::DragFloat3("基準座標 (追従しないとき)", &c.basePosition.x, 0.05f);
            ImGui::DragFloat3("発生位置のずれ", &c.positionOffset.x, 0.05f);
            BindingRegistry::DrawPositionCombo("目標を追従", c.targetKey);
            ImGui::DragFloat3("目標のずれ", &c.targetOffset.x, 0.05f);
            BindingRegistry::DrawFlagCombo("再生のきっかけ(フラグ)", c.triggerFlagKey);
            ImGui::Checkbox("フラグが下りたら止める (連続)", &c.stopWhenFlagOff);
            const Vector3 origin = ResolveOrigin(effect);
            const Vector3 target = ResolveTarget(effect, origin);
            ImGui::Text("今の発生位置 : (%.2f, %.2f, %.2f)", origin.x, origin.y, origin.z);
            ImGui::Text("今の目標     : (%.2f, %.2f, %.2f)", target.x, target.y, target.z);
        }

        // --- 形 ---
        if (ImGui::CollapsingHeader("発生する場所の形")) {
            ComboEnum("形", c.spawnShape, N::kSpawnShapeLabels);
            switch (c.spawnShape) {
            case ParticleSpawnShape::Sphere:
            case ParticleSpawnShape::Ring:
            case ParticleSpawnShape::Cylinder:
                ImGui::DragFloat("半径", &c.radius, 0.01f, 0.0f, 100.0f);
                ImGui::SliderFloat("内側の割合 (1:外周だけ 0:中まで)", &c.innerRatio, 0.0f, 1.0f);
                if (c.spawnShape == ParticleSpawnShape::Cylinder) {
                    ImGui::DragFloat("高さ", &c.height, 0.01f, 0.0f, 100.0f);
                }
                if (c.spawnShape != ParticleSpawnShape::Sphere) {
                    ImGui::Checkbox("軸をカメラに向ける (画面に正対)", &c.axisFacesCamera);
                    if (!c.axisFacesCamera) {
                        ImGui::DragFloat3("軸", &c.shapeAxis.x, 0.01f, -1.0f, 1.0f);
                    }
                }
                ImGui::DragFloat("半径が縮む時間(秒) 0で縮まない", &c.shrinkTime, 0.01f, 0.0f, 30.0f);
                if (c.shrinkTime > 0.0f) {
                    ImGui::SliderFloat("縮み終わりの半径の割合", &c.shrinkEndRatio, 0.0f, 2.0f);
                }
                break;
            case ParticleSpawnShape::Line:
                ImGui::DragFloat3("終点 (発生位置から)", &c.lineEnd.x, 0.05f);
                break;
            case ParticleSpawnShape::Box:
                ImGui::DragFloat3("大きさ", &c.boxSize.x, 0.05f, 0.0f, 1000.0f);
                break;
            default:
                ImGui::TextDisabled("1点から出ます");
                break;
            }
        }

        // --- 初速 ---
        if (ImGui::CollapsingHeader("初速")) {
            if (c.motion == ParticleMotion::Explode) {
                DragRange("速さ(最小/最大)", c.speedMin, c.speedMax, 0.05f, 0.0f, 1000.0f);
            } else if (c.motion == ParticleMotion::Physics || c.motion == ParticleMotion::Homing) {
                ComboEnum("初速の決め方", c.velocityMode, N::kVelocityModeLabels);
                switch (c.velocityMode) {
                case ParticleVelocityMode::Fixed:
                    ImGui::DragFloat3("速度", &c.velocity.x, 0.05f);
                    break;
                case ParticleVelocityMode::Random:
                    ImGui::DragFloat3("速度(最小)", &c.velocityMin.x, 0.05f);
                    ImGui::DragFloat3("速度(最大)", &c.velocityMax.x, 0.05f);
                    break;
                default:
                    DragRange("速さ(最小/最大)", c.speedMin, c.speedMax, 0.05f, 0.0f, 1000.0f);
                    break;
                }
            } else {
                ImGui::TextDisabled("この動き方では初速は使いません");
            }
        }

        // --- 寿命・大きさ・回転 ---
        if (ImGui::CollapsingHeader("寿命・大きさ・回転")) {
            DragRange("寿命(秒) 最小/最大", c.lifeMin, c.lifeMax, 0.01f, 0.01f, 60.0f);
            DragRange("大きさ 最小/最大", c.scaleMin, c.scaleMax, 0.005f, 0.0f, 100.0f);
            DragRange("大きさの倍率 始め/終わり", c.startScale, c.endScale, 0.01f, 0.0f, 10.0f);
            ImGui::Checkbox("向きをばらばらにする", &c.randomRotation);
            DragRange("回転の速さ(度/秒) 最小/最大", c.rotationSpeedMin, c.rotationSpeedMax, 1.0f, -3600.0f, 3600.0f);
        }

        // --- 動き方 ---
        if (ImGui::CollapsingHeader("動き方", ImGuiTreeNodeFlags_DefaultOpen)) {
            ComboEnum("種類##motion", c.motion, N::kMotionLabels);
            const bool usesMotionAxis = (c.motion == ParticleMotion::Converge || c.motion == ParticleMotion::Vortex);
            if (usesMotionAxis) {
                ImGui::Checkbox("軸をカメラに向ける##motion", &c.axisFacesCamera);
                if (!c.axisFacesCamera) {
                    ImGui::DragFloat3("回転軸", &c.motionAxis.x, 0.01f, -1.0f, 1.0f);
                }
            }
            switch (c.motion) {
            case ParticleMotion::Converge:
                ImGui::DragFloat("到着までに回る角度(度)", &c.arcAngle, 1.0f, -1440.0f, 1440.0f);
                ImGui::Checkbox("半分は逆回り", &c.arcBothDirections);
                ImGui::DragFloat("加速の強さ (1:等速)", &c.easePower, 0.01f, 0.1f, 10.0f);
                break;
            case ParticleMotion::Vortex:
                ImGui::DragFloat("角速度(度/秒)", &c.vortexAngularSpeed, 1.0f, -3600.0f, 3600.0f);
                ImGui::DragFloat("半径の変化(負で吸い込み)", &c.vortexRadialSpeed, 0.01f, -50.0f, 50.0f);
                ImGui::DragFloat("軸方向の速さ", &c.vortexRiseSpeed, 0.01f, -50.0f, 50.0f);
                break;
            case ParticleMotion::Homing:
                ImGui::DragFloat("追尾の速さ", &c.homingSpeed, 0.05f, 0.0f, 200.0f);
                ImGui::DragFloat("曲がりやすさ", &c.homingTurn, 0.01f, 0.0f, 50.0f);
                ImGui::DragFloat("到着とみなす距離", &c.arriveRadius, 0.01f, 0.0f, 10.0f);
                break;
            case ParticleMotion::Bezier:
                ImGui::DragFloat("加速の強さ (1:等速)", &c.easePower, 0.01f, 0.1f, 10.0f);
                ImGui::DragFloat3("制御点のずれ(最小)", &c.controlOffsetMin.x, 0.05f);
                ImGui::DragFloat3("制御点のずれ(最大)", &c.controlOffsetMax.x, 0.05f);
                break;
            default:
                break;
            }

            const bool usesPhysics = (c.motion == ParticleMotion::Physics || c.motion == ParticleMotion::Explode);
            if (usesPhysics) {
                ImGui::DragFloat("重力", &c.gravity, 0.05f, -200.0f, 200.0f);
                ImGui::DragFloat("空気抵抗", &c.drag, 0.01f, 0.0f, 50.0f);
                ImGui::Checkbox("床で跳ねる", &c.bounce);
                if (c.bounce) {
                    ImGui::DragFloat("床の高さ", &c.floorY, 0.01f);
                    ImGui::SliderFloat("跳ね返り", &c.restitution, 0.0f, 1.0f);
                    ImGui::SliderFloat("跳ねたときの横の減速", &c.friction, 0.0f, 1.0f);
                }
            }
            if (usesPhysics || c.motion == ParticleMotion::Homing) {
                ImGui::DragFloat("揺らぎの強さ", &c.noiseStrength, 0.05f, 0.0f, 200.0f);
                ImGui::DragFloat("揺らぎの細かさ", &c.noiseFrequency, 0.01f, 0.0f, 20.0f);
            }
            ImGui::DragFloat("ジッターの大きさ", &c.jitterStrength, 0.005f, 0.0f, 10.0f);
            if (c.jitterStrength > 0.0f) {
                ImGui::DragFloat("ジッターの回数(回/秒)", &c.jitterFrequency, 0.1f, 0.0f, 120.0f);
            }
        }

        // --- 消え方 ---
        if (ImGui::CollapsingHeader("消え方", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ComboEnum("種類##death", c.death, N::kDeathLabels)) {
                needApply = true; // 分裂を使うかどうかで、確保するバッファが変わる
            }
            if (c.death != ParticleDeath::Split) {
                ImGui::SliderFloat("寿命の最後の何割で消すか", &c.deathRatio, 0.0f, 1.0f);
            }
            switch (c.death) {
            case ParticleDeath::Blink:
                DragRange("点滅の回数(回/秒) 最小/最大", c.blinkFrequencyMin, c.blinkFrequencyMax, 0.1f, 0.0f, 120.0f);
                ImGui::SliderFloat("一緒に薄くする量", &c.blinkFade, 0.0f, 1.0f);
                break;
            case ParticleDeath::Stretch:
                ImGui::DragFloat("伸び量", &c.deathStretch, 0.01f, 0.0f, 50.0f);
                break;
            case ParticleDeath::ColorShift:
                ImGui::ColorEdit4("途中の色", &c.deathColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                break;
            case ParticleDeath::Split:
                DragU32("分裂する数", c.splitCount, 0.1f, 0, 64);
                ImGui::DragFloat("分裂の速さ", &c.splitSpeed, 0.05f, 0.0f, 200.0f);
                ImGui::DragFloat("分裂した粒の寿命(秒)", &c.splitLife, 0.01f, 0.01f, 30.0f);
                ImGui::DragFloat("分裂した粒の大きさの倍率", &c.splitScale, 0.01f, 0.0f, 10.0f);
                ImGui::TextDisabled("最大数は「親の数 x (1 + 分裂する数)」より多めに");
                break;
            case ParticleDeath::Absorb:
                ImGui::DragFloat("吸い込む強さ", &c.absorbStrength, 0.1f, 0.0f, 500.0f);
                ImGui::TextDisabled("吸い込み先は「目標」です");
                break;
            default:
                break;
            }
        }

        // --- 見た目 ---
        if (ImGui::CollapsingHeader("見た目", ImGuiTreeNodeFlags_DefaultOpen)) {
            ComboEnum("1粒の形", c.look, N::kLookLabels);
            if (c.look == ParticleLook::Texture) {
                if (textureBufferOwner_ != name) {
                    strcpy_s(texturePathBuffer_, sizeof(texturePathBuffer_), c.texturePath.c_str());
                    textureBufferOwner_ = name;
                }
                const bool entered = ImGui::InputText("画像 (名前かパス)", texturePathBuffer_, sizeof(texturePathBuffer_), ImGuiInputTextFlags_EnterReturnsTrue);
                if (entered || ImGui::IsItemDeactivatedAfterEdit()) {
                    c.texturePath = texturePathBuffer_;
                    needApply = true;
                }
            } else {
                ImGui::DragFloat("ふちのぼけ", &c.softness, 0.01f, 0.01f, 10.0f);
            }
            ComboEnum("ブレンド", c.blend, N::kBlendLabels);
            const ImGuiColorEditFlags colorFlags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
            ImGui::ColorEdit4("色A", &c.colorA.x, colorFlags);
            ImGui::ColorEdit4("色B (A〜Bで乱数)", &c.colorB.x, colorFlags);
            ImGui::ColorEdit4("終わりに掛ける色", &c.endColor.x, colorFlags);
            ImGui::DragFloat("明るさ (1超えで光る)", &c.intensity, 0.01f, 0.0f, 100.0f);
            ImGui::DragFloat("速度方向への伸び", &c.stretch, 0.001f, 0.0f, 10.0f);
            ImGui::DragFloat("速いほど明るく", &c.speedBrightness, 0.001f, 0.0f, 10.0f);
        }

        // --- 連鎖 ---
        if (ImGui::CollapsingHeader("連鎖 (続けて再生)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("再生してから指定の秒数後に、別のエフェクトを再生します");
            int removeIndex = -1;
            for (size_t i = 0; i < c.followUps.size(); ++i) {
                ParticleFollowUp& followUp = c.followUps[i];
                ImGui::PushID(static_cast<int>(i));

                // 再生するエフェクト (一覧から選ぶ)
                ImGui::SetNextItemWidth(listNameWidth_);
                const char* preview = followUp.effectName.empty() ? "(なし)" : followUp.effectName.c_str();
                if (ImGui::BeginCombo("##followUpEffect", preview)) {
                    for (const auto& [otherName, other] : effects_) {
                        if (ImGui::Selectable(otherName.c_str(), otherName == followUp.effectName)) {
                            followUp.effectName = otherName;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(followUpDelayWidth_);
                ImGui::DragFloat("秒後##followUpDelay", &followUp.delay, 0.01f, 0.0f, 60.0f);
                ImGui::SameLine();
                ImGui::Checkbox("同じ場所##followUpPosition", &followUp.samePosition);
                ImGui::SameLine();
                if (ImGui::SmallButton("削除##followUpRemove")) {
                    removeIndex = static_cast<int>(i);
                }
                if (!followUp.effectName.empty() && !effects_.contains(followUp.effectName)) {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "「%s」というエフェクトがありません", followUp.effectName.c_str());
                }
                ImGui::PopID();
            }
            if (removeIndex >= 0) {
                c.followUps.erase(c.followUps.begin() + removeIndex);
            }
            if (ImGui::Button("連鎖を追加")) {
                c.followUps.push_back(ParticleFollowUp{});
            }
        }

        ImGui::PopID();

        if (needApply) {
            ApplyConfig(effect, false, SumMaxParticlesExcept(&effect));
        }

        // 一覧を書き換える操作は最後に (effect への参照を使い終わってから)
        if (requestDuplicate) {
            std::string newName = name + "_copy";
            for (int number = 2; effects_.contains(newName); ++number) {
                newName = name + "_copy" + std::to_string(number);
            }
            const ParticleConfig copied = c;
            if (Create(newName, copied)) {
                selectedName_ = newName;
            }
        }
        if (requestDestroy) {
            const std::string target = name; // name は消える要素のキーへの参照なので、先にコピーする
            Destroy(target);
            selectedName_.clear();
        }
    }
#endif
}
