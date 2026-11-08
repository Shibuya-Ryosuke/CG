#include "GPUParticleManager.h"
#include "../../Core/Base/Logger.h"
#include "../../Camera/CameraManager.h"
#include "../../Core/Base/BindingRegistry.h"

#ifdef NRELEASE
#include <imgui.h>
#endif

#pragma warning(disable: 4866)

#include <RyoEngine.h>
#include <fstream>
#include <vector>
#include <utility>
#include <json.hpp>

namespace RyoEngine {

    const std::string GPUParticleManager::kFileName = "gpu_particle_presets.json";

    GPUParticleManager* GPUParticleManager::GetInstance() {
        static GPUParticleManager instance;
        return &instance;
    }

    void GPUParticleManager::Initialize() {
        GPUParticleManager* instance = GetInstance();

        Logger::Log("GPUParticleManager : Initializing...\n");
#ifdef NRELEASE
        strcpy_s(instance->folderPathBuffer_, sizeof(instance->folderPathBuffer_), instance->folderPath_.c_str());
#endif
        Logger::LogSuccess("GPUParticleManager : Initialized\n");
    }

    void GPUParticleManager::Finalize() {
        GPUParticleManager* instance = GetInstance();

        Logger::Log("GPUParticleManager : Finalizing...\n");
        for (auto& [name, managed] : instance->emitters_) {
            managed->emitter.Finalize();
        }
        instance->emitters_.clear();
        Logger::LogSuccess("GPUParticleManager : Finalized\n");
    }

    void GPUParticleManager::ApplyConfig(ManagedEmitter& managed) {
        // NOTE: メッシュ・最大数・ビルボード・ブレンドモードのいずれかが変わった場合
        //       GPUParticleEmitterは内部バッファを作り直す必要があるため、一旦Finalize()してから
        //       Initialize()し直す(初回生成時もFinalize()は安全な空振りになる)。
        managed.emitter.Finalize();
        // meshPathが空ならデフォルトのモデル(四角)を使う
        const std::string& meshPath = managed.config.meshPath.empty() ? defaultMeshPath_ : managed.config.meshPath;
        managed.emitter.Initialize(meshPath, managed.config.maxParticleCount,
            managed.config.billboard, managed.config.blendMode);
        managed.emitter.SetGravity(managed.config.gravity);
        managed.emitter.SetTexture(managed.config.texturePath);
    }

    void GPUParticleManager::EmitOnce(ManagedEmitter& managed) {
        const GPUParticleEmitterConfig& config = managed.config;

        const Vector3* tracked = BindingRegistry::FindPosition(config.trackedPositionKey);
        Vector3 basePosition = tracked ? *tracked : config.basePosition;
        Vector3 positionOffset = {
            RandomFloat(config.positionOffsetRandomMin.x, config.positionOffsetRandomMax.x),
            RandomFloat(config.positionOffsetRandomMin.y, config.positionOffsetRandomMax.y),
            RandomFloat(config.positionOffsetRandomMin.z, config.positionOffsetRandomMax.z),
        };
        Vector3 position = basePosition + positionOffset;

        Vector3 velocity = config.velocity;
        if (config.useRandomVelocity) {
            velocity = {
                RandomFloat(config.velocityRandomMin.x, config.velocityRandomMax.x),
                RandomFloat(config.velocityRandomMin.y, config.velocityRandomMax.y),
                RandomFloat(config.velocityRandomMin.z, config.velocityRandomMax.z),
            };
        }

        managed.emitter.Emit(position, velocity, config.color, config.scale, config.fixedRotation, config.lifeTime);
    }

    bool GPUParticleManager::CreateEmitter(const std::string& name, const GPUParticleEmitterConfig& config) {
        GPUParticleManager* instance = GetInstance();

        if (instance->emitters_.contains(name)) {
            Logger::Log("[GPUParticleManager] CreateEmitter: 同名のEmitterが既に存在します: " + name + "\n");
            return false;
        }
        if (name.empty()) {
            Logger::Log("[GPUParticleManager] CreateEmitter: 名前が空です\n");
            return false;
        }

        auto managed = std::make_unique<ManagedEmitter>();
        managed->config = config;
#ifdef NRELEASE
        strcpy_s(managed->meshPathBuffer, sizeof(managed->meshPathBuffer), config.meshPath.c_str());
        strcpy_s(managed->texturePathBuffer, sizeof(managed->texturePathBuffer), config.texturePath.c_str());
#endif
        managed->useCustomMesh = !config.meshPath.empty();

        instance->ApplyConfig(*managed);

        instance->emitters_[name] = std::move(managed);
        return true;
    }

    void GPUParticleManager::DestroyEmitter(const std::string& name) {
        GPUParticleManager* instance = GetInstance();

        auto it = instance->emitters_.find(name);
        if (it == instance->emitters_.end()) {
            return;
        }
        it->second->emitter.Finalize();
        instance->emitters_.erase(it);
    }

    void GPUParticleManager::RegisterPosition(const std::string& key, const Vector3* pos) {
        BindingRegistry::RegisterPosition(key, pos);
    }

    void GPUParticleManager::UnregisterPosition(const std::string& key) {
        BindingRegistry::UnregisterPosition(key);
    }

    void GPUParticleManager::RegisterFlag(const std::string& key, const bool* flag) {
        BindingRegistry::RegisterFlag(key, flag);
    }

    void GPUParticleManager::UnregisterFlag(const std::string& key) {
        BindingRegistry::UnregisterFlag(key);
    }

    void GPUParticleManager::ClearRegistrations() {
        BindingRegistry::Clear();
    }

    void GPUParticleManager::SetTrackedPositionKey(const std::string& emitterName, const std::string& key) {
        GPUParticleManager* instance = GetInstance();

        auto it = instance->emitters_.find(emitterName);
        if (it == instance->emitters_.end()) {
            Logger::Log("[GPUParticleManager] SetTrackedPositionKey: 該当するEmitterがありません: " + emitterName + "\n");
            return;
        }
        it->second->config.trackedPositionKey = key;
    }

    void GPUParticleManager::SetConditionFlagKey(const std::string& emitterName, const std::string& key) {
        GPUParticleManager* instance = GetInstance();

        auto it = instance->emitters_.find(emitterName);
        if (it == instance->emitters_.end()) {
            Logger::Log("[GPUParticleManager] SetConditionFlagKey: 該当するEmitterがありません: " + emitterName + "\n");
            return;
        }
        it->second->config.conditionFlagKey = key;
    }

    void GPUParticleManager::TriggerEmit(const std::string& name) {
        GPUParticleManager* instance = GetInstance();

        auto it = instance->emitters_.find(name);
        if (it == instance->emitters_.end()) {
            return;
        }
        instance->EmitOnce(*it->second);
    }

    void GPUParticleManager::Update(float deltaTime) {
        GPUParticleManager* instance = GetInstance();

        for (auto& [name, managed] : instance->emitters_) {
            bool shouldEmit = false;

            const bool* flag = BindingRegistry::FindFlag(managed->config.conditionFlagKey);

            // 参照先が変わった(キー変更・登録・解除があった)フレームは、
            // 偽の「立ち上がり」を検出しないよう、現在値で前回状態を合わせ直す
            if (flag != managed->lastFlagPtr) {
                managed->lastFlagPtr = flag; // 比較にしか使わない(参照はしない)
                managed->previousFlagState = flag ? (*flag == managed->config.emitWhenTrue) : false;
            }

            if (flag != nullptr) {
                // 「発生する条件」を満たしているか (trueのときに発生 / falseのときに発生 を選べる)
                bool current = (*flag == managed->config.emitWhenTrue);

                if (managed->config.continuousWhileTrue) {
                    if (current) {
                        managed->emitTimer -= deltaTime;
                        if (managed->emitTimer <= 0.0f) {
                            shouldEmit = true;
                            managed->emitTimer = managed->config.emitInterval;
                        }
                    }
                } else {
                    // 立ち上がりの瞬間だけ1回
                    if (current && !managed->previousFlagState) {
                        shouldEmit = true;
                    }
                }
                managed->previousFlagState = current;
            }

            if (shouldEmit) {
                instance->EmitOnce(*managed);
            }

            managed->emitter.Update(deltaTime);
        }
    }

    void GPUParticleManager::Draw() {
        GPUParticleManager* instance = GetInstance();

        // CameraManager::UpdateAll()で確定済みの描画カメラを使う(Modelのカリングと同じカメラになる)
        const Camera& camera = CameraManager::GetActive();
        for (auto& [name, managed] : instance->emitters_) {
            managed->emitter.Draw(camera);
        }
    }

    void GPUParticleManager::SetFolderPath(const std::string& path) {
        GPUParticleManager* instance = GetInstance();

        instance->folderPath_ = path;
        if (!instance->folderPath_.empty() && instance->folderPath_.back() != '/') {
            instance->folderPath_ += '/';
        }
#ifdef NRELEASE
        // ImGui用の入力バッファも同期する
        strcpy_s(instance->folderPathBuffer_, sizeof(instance->folderPathBuffer_), instance->folderPath_.c_str());
#endif
    }

    namespace {
        using json = nlohmann::json;

        json ToJson(const Vector3& v) { return json::array({ v.x, v.y, v.z }); }
        json ToJson(const Vector4& v) { return json::array({ v.x, v.y, v.z, v.w }); }

        // キーが無い・形が違う場合はdefaultValueを返す(古いファイルや手書きの欠けにも強くするため)
        Vector3 ReadVector3(const json& j, const char* key, const Vector3& defaultValue) {
            auto it = j.find(key);
            if (it == j.end() || !it->is_array() || it->size() < 3) {
                return defaultValue;
            }
            return Vector3{ (*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>() };
        }

        Vector4 ReadVector4(const json& j, const char* key, const Vector4& defaultValue) {
            auto it = j.find(key);
            if (it == j.end() || !it->is_array() || it->size() < 4) {
                return defaultValue;
            }
            return Vector4{ (*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>(), (*it)[3].get<float>() };
        }

        json ConfigToJson(const GPUParticleEmitterConfig& c) {
            json j;
            j["meshPath"] = c.meshPath;
            j["texturePath"] = c.texturePath;
            j["maxParticleCount"] = c.maxParticleCount;
            j["scale"] = c.scale;
            j["lifeTime"] = c.lifeTime;
            j["gravity"] = c.gravity;
            j["color"] = ToJson(c.color);
            j["blendMode"] = (c.blendMode == GPUParticleCommon::BlendMode::Additive) ? "Additive" : "Alpha";
            j["billboard"] = c.billboard;
            j["fixedRotation"] = ToJson(c.fixedRotation);

            j["useRandomVelocity"] = c.useRandomVelocity;
            j["velocity"] = ToJson(c.velocity);
            j["velocityRandomMin"] = ToJson(c.velocityRandomMin);
            j["velocityRandomMax"] = ToJson(c.velocityRandomMax);

            j["trackedPositionKey"] = c.trackedPositionKey;
            j["basePosition"] = ToJson(c.basePosition);
            j["positionOffsetRandomMin"] = ToJson(c.positionOffsetRandomMin);
            j["positionOffsetRandomMax"] = ToJson(c.positionOffsetRandomMax);

            j["conditionFlagKey"] = c.conditionFlagKey;
            j["emitWhenTrue"] = c.emitWhenTrue;
            j["continuousWhileTrue"] = c.continuousWhileTrue;
            j["emitInterval"] = c.emitInterval;
            return j;
        }

        // 型が合わない値があるとjson::exceptionが投げられる(呼び出し側でcatchする)
        GPUParticleEmitterConfig ConfigFromJson(const json& j) {
            const GPUParticleEmitterConfig def{};
            GPUParticleEmitterConfig c;
            c.meshPath = j.value("meshPath", def.meshPath);
            c.texturePath = j.value("texturePath", def.texturePath);
            c.maxParticleCount = j.value("maxParticleCount", def.maxParticleCount);
            c.scale = j.value("scale", def.scale);
            c.lifeTime = j.value("lifeTime", def.lifeTime);
            c.gravity = j.value("gravity", def.gravity);
            c.color = ReadVector4(j, "color", def.color);

            std::string blend = j.value("blendMode", std::string("Additive"));
            c.blendMode = (blend == "Alpha") ? GPUParticleCommon::BlendMode::Alpha : GPUParticleCommon::BlendMode::Additive;

            c.billboard = j.value("billboard", def.billboard);
            c.fixedRotation = ReadVector3(j, "fixedRotation", def.fixedRotation);

            c.useRandomVelocity = j.value("useRandomVelocity", def.useRandomVelocity);
            c.velocity = ReadVector3(j, "velocity", def.velocity);
            c.velocityRandomMin = ReadVector3(j, "velocityRandomMin", def.velocityRandomMin);
            c.velocityRandomMax = ReadVector3(j, "velocityRandomMax", def.velocityRandomMax);

            c.trackedPositionKey = j.value("trackedPositionKey", def.trackedPositionKey);
            c.basePosition = ReadVector3(j, "basePosition", def.basePosition);
            c.positionOffsetRandomMin = ReadVector3(j, "positionOffsetRandomMin", def.positionOffsetRandomMin);
            c.positionOffsetRandomMax = ReadVector3(j, "positionOffsetRandomMax", def.positionOffsetRandomMax);

            c.conditionFlagKey = j.value("conditionFlagKey", def.conditionFlagKey);
            c.emitWhenTrue = j.value("emitWhenTrue", def.emitWhenTrue);
            c.continuousWhileTrue = j.value("continuousWhileTrue", def.continuousWhileTrue);
            c.emitInterval = j.value("emitInterval", def.emitInterval);
            return c;
        }
    }

    void GPUParticleManager::SaveToFileInternal(const std::string& fullPath) {
        std::ofstream file(fullPath);
        if (!file.is_open()) {
            Logger::Log("[GPUParticleManager] Save: ファイルを開けませんでした: " + fullPath + "\n");
            return;
        }

        json root;
        root["version"] = 1;
        root["emitters"] = json::object();
        for (const auto& [name, managed] : emitters_) {
            root["emitters"][name] = ConfigToJson(managed->config);
        }

        // 不正なUTF-8が混ざっていても例外にならないよう、置換モードで出力する
        file << root.dump(4, ' ', false, json::error_handler_t::replace) << "\n";

        Logger::LogSuccess("[GPUParticleManager] Save: 保存しました: " + fullPath + "\n");
    }

    void GPUParticleManager::Save() {
        GPUParticleManager* instance = GetInstance();

        std::string fullPath = GetFullFilePath();
#ifdef NRELEASE
        if (std::filesystem::exists(fullPath)) {
            instance->showOverwriteModal_ = true;
        } else {
            instance->SaveToFileInternal(fullPath);
        }
#else
        instance->SaveToFileInternal(fullPath);
#endif
    }

    void GPUParticleManager::Load() {
        GPUParticleManager* instance = GetInstance();

        std::string fullPath = instance->folderPath_ + kFileName;
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            Logger::LogError("[GPUParticleManager] Load: ファイルを開けませんでした: " + fullPath + "\n");
            return;
        }

        json root = json::parse(file, nullptr, false); // 第3引数false: 失敗時に例外ではなくdiscardedを返す
        if (root.is_discarded() || !root.is_object() || !root.contains("emitters") || !root["emitters"].is_object()) {
            Logger::Log("[GPUParticleManager] Load: JSONの形式が不正です: " + fullPath + "\n");
            return;
        }

        // 既存のEmitterを壊す前に、先に全部パースしておく(途中で失敗しても今の状態を保てる)
        std::vector<std::pair<std::string, GPUParticleEmitterConfig>> loaded;
        try {
            const json& emittersJson = root["emitters"];
            for (auto it = emittersJson.begin(); it != emittersJson.end(); ++it) {
                loaded.emplace_back(it.key(), ConfigFromJson(it.value()));
            }
        }
        catch (const json::exception& e) {
            Logger::Log("[GPUParticleManager] Load: 読み込みに失敗しました: " + std::string(e.what()) + "\n");
            return;
        }

        // 読み込んだ内容で作り直すため、既存のEmitterは一旦全部破棄する
        for (auto& [name, managed] : instance->emitters_) {
            managed->emitter.Finalize();
        }
        instance->emitters_.clear();

        for (auto& [name, config] : loaded) {
            // デフォルトのモデル(四角)のパスが書かれていたら、空(=デフォルト)にそろえる
            // (古いJSONや、デフォルトの置き場所を移した場合にも読めるようにするため)
            if (config.meshPath == instance->defaultMeshPath_ || config.meshPath == instance->oldDefaultMeshPath_) {
                config.meshPath.clear();
            }
            CreateEmitter(name, config);
        }

        Logger::LogSuccess("[GPUParticleManager] Load: 読み込みました: " + fullPath + "\n");
    }

    void GPUParticleManager::DrawImGui() {
        GPUParticleManager* instance = GetInstance();

#ifdef NRELEASE
        ImGui::Begin("GPUParticleManager");

        // --- 保存・読込エリア ---
        ImGui::Text("保存・読込先");
        if (instance->folderPathBuffer_[0] == '\0' && !instance->folderPath_.empty()) {
            strcpy_s(instance->folderPathBuffer_, sizeof(instance->folderPathBuffer_), instance->folderPath_.c_str());
        }
        bool folderEnter = ImGui::InputText("フォルダパス", instance->folderPathBuffer_, sizeof(instance->folderPathBuffer_), ImGuiInputTextFlags_EnterReturnsTrue);
        if (folderEnter || ImGui::IsItemDeactivatedAfterEdit()) {
            SetFolderPath(instance->folderPathBuffer_);
        }
        ImGui::Text("ファイル名 : %s", kFileName.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Save")) {
            SetFolderPath(instance->folderPathBuffer_);
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            SetFolderPath(instance->folderPathBuffer_);
            Load();
        }

        ImGui::Separator();

        // モデルの選び方 (0:デフォルトのモデル(四角) / 1:objパスを入力する)
        const char* meshModeNames[] = { "デフォルト(四角)", "カスタム" };

        // --- 新規作成フォーム ---
        if (ImGui::CollapsingHeader("新規Emitter作成")) {
            ImGui::InputText("名前", instance->newNameBuffer_, sizeof(instance->newNameBuffer_));
            int newMeshMode = instance->newUseCustomMesh_ ? 1 : 0;
            if (ImGui::Combo("モデル", &newMeshMode, meshModeNames, IM_ARRAYSIZE(meshModeNames))) {
                instance->newUseCustomMesh_ = (newMeshMode == 1);
            }
            if (instance->newUseCustomMesh_) {
                ImGui::InputText("objパス", instance->newMeshPathBuffer_, sizeof(instance->newMeshPathBuffer_));
            }
            ImGui::InputText("テクスチャパス(空可)", instance->newTexturePathBuffer_, sizeof(instance->newTexturePathBuffer_));
            ImGui::InputInt("最大数", &instance->newMaxParticleCount_);
            if (instance->newMaxParticleCount_ < 1) {
                instance->newMaxParticleCount_ = 1;
            }

            if (ImGui::Button("作成")) {
                GPUParticleEmitterConfig config{};
                // デフォルトを選んでいるときは空(=デフォルトのモデル)
                config.meshPath = instance->newUseCustomMesh_ ? instance->newMeshPathBuffer_ : "";
                config.texturePath = instance->newTexturePathBuffer_;
                config.maxParticleCount = static_cast<uint32_t>(instance->newMaxParticleCount_);

                if (CreateEmitter(instance->newNameBuffer_, config)) {
                    instance->newNameBuffer_[0] = '\0';
                    instance->newMeshPathBuffer_[0] = '\0';
                    instance->newUseCustomMesh_ = false;
                    instance->newTexturePathBuffer_[0] = '\0';
                    instance->newMaxParticleCount_ = 500;
                }
            }
        }

        ImGui::Separator();

        // --- 各Emitterの設定 ---
        std::string nameToDestroy;
        for (auto& [name, managed] : instance->emitters_) {
            ImGui::PushID(name.c_str());
            if (ImGui::CollapsingHeader(name.c_str())) {
                GPUParticleEmitterConfig& config = managed->config;
                bool needReapply = false;

                // モデル (デフォルト / カスタム)
                int meshMode = managed->useCustomMesh ? 1 : 0;
                if (ImGui::Combo("モデル", &meshMode, meshModeNames, IM_ARRAYSIZE(meshModeNames))) {
                    managed->useCustomMesh = (meshMode == 1);
                    // デフォルトなら空にする。カスタムなら、前に入力してあったパスがあればそれを使う
                    config.meshPath = managed->useCustomMesh ? managed->meshPathBuffer : "";
                    needReapply = true;
                }
                if (managed->useCustomMesh) {
                    // objパス (Enter/フォーカス外れで確定)
                    bool meshEnter = ImGui::InputText("objパス", managed->meshPathBuffer, sizeof(managed->meshPathBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
                    if (meshEnter || ImGui::IsItemDeactivatedAfterEdit()) {
                        config.meshPath = managed->meshPathBuffer;
                        needReapply = true;
                    }
                    if (config.meshPath.empty()) {
                        ImGui::TextDisabled("パスを入力するまでは、デフォルトのモデルを使います");
                    }
                }

                // テクスチャパス (こちらはメッシュと違って再生成不要。即座に反映できる)
                bool texEnter = ImGui::InputText("テクスチャパス(空可)", managed->texturePathBuffer, sizeof(managed->texturePathBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
                if (texEnter || ImGui::IsItemDeactivatedAfterEdit()) {
                    config.texturePath = managed->texturePathBuffer;
                    managed->emitter.SetTexture(config.texturePath);
                }

                int maxCount = static_cast<int>(config.maxParticleCount);
                ImGui::InputInt("最大数", &maxCount);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    if (maxCount < 1) {
                        maxCount = 1;
                    }
                    config.maxParticleCount = static_cast<uint32_t>(maxCount);
                    needReapply = true;
                }

                ImGui::DragFloat("スケール", &config.scale, 0.01f, 0.01f, 100.0f);
                ImGui::DragFloat("寿命(秒)", &config.lifeTime, 0.01f, 0.01f, 60.0f);
                if (ImGui::DragFloat("重力", &config.gravity, 0.01f, -50.0f, 50.0f)) {
                    managed->emitter.SetGravity(config.gravity);
                }

                ImGui::ColorEdit4("色", &config.color.x);

                const char* blendNames[] = { "通常(Alpha)", "加算(Additive)" };
                int blendIndex = (config.blendMode == GPUParticleCommon::BlendMode::Additive) ? 1 : 0;
                if (ImGui::Combo("ブレンドモード", &blendIndex, blendNames, IM_ARRAYSIZE(blendNames))) {
                    config.blendMode = (blendIndex == 1) ? GPUParticleCommon::BlendMode::Additive : GPUParticleCommon::BlendMode::Alpha;
                    needReapply = true;
                }

                if (ImGui::Checkbox("ビルボード", &config.billboard)) {
                    needReapply = true;
                }
                if (!config.billboard) {
                    ImGui::DragFloat3("固定姿勢(ラジアン)", &config.fixedRotation.x, 0.01f);
                }

                ImGui::Separator();
                ImGui::Text("速度");
                ImGui::Checkbox("速度を乱数にする", &config.useRandomVelocity);
                if (config.useRandomVelocity) {
                    ImGui::DragFloat3("速度(最小)", &config.velocityRandomMin.x, 0.01f);
                    ImGui::DragFloat3("速度(最大)", &config.velocityRandomMax.x, 0.01f);
                } else {
                    ImGui::DragFloat3("速度(固定)", &config.velocity.x, 0.01f);
                }

                ImGui::Separator();
                ImGui::Text("発生位置");
                BindingRegistry::DrawPositionCombo("追従する座標", config.trackedPositionKey);
                if (const Vector3* tracked = BindingRegistry::FindPosition(config.trackedPositionKey)) {
                    ImGui::Text("追従中の座標: (%.2f, %.2f, %.2f)", tracked->x, tracked->y, tracked->z);
                } else {
                    ImGui::DragFloat3("基準座標", &config.basePosition.x, 0.1f);
                }
                ImGui::DragFloat3("座標オフセット乱数(最小)", &config.positionOffsetRandomMin.x, 0.01f);
                ImGui::DragFloat3("座標オフセット乱数(最大)", &config.positionOffsetRandomMax.x, 0.01f);

                ImGui::Separator();
                ImGui::Text("発生条件");
                BindingRegistry::DrawFlagCombo("条件フラグ", config.conditionFlagKey);
                if (const bool* flag = BindingRegistry::FindFlag(config.conditionFlagKey)) {
                    ImGui::Text("現在の値: %s", *flag ? "true" : "false");
                } else {
                    ImGui::TextDisabled("条件フラグ未選択(「今すぐ1個発生」での手動発生のみ)");
                }
                // フラグが true のときに発生させるか、false のときに発生させるか
                const char* whenNames[] = { "trueのとき", "falseのとき" };
                int whenIndex = config.emitWhenTrue ? 0 : 1;
                if (ImGui::Combo("発生する条件", &whenIndex, whenNames, IM_ARRAYSIZE(whenNames))) {
                    config.emitWhenTrue = (whenIndex == 0);
                    // 切り替えた瞬間に「条件を満たした」と誤って1回出ないよう、次のUpdateで前回状態を合わせ直させる
                    managed->lastFlagPtr = nullptr;
                }
                ImGui::Checkbox("条件を満たしている間ずっと発生", &config.continuousWhileTrue);
                if (config.continuousWhileTrue) {
                    ImGui::DragFloat("発生間隔(秒)", &config.emitInterval, 0.01f, 0.0f, 10.0f);
                }

                if (ImGui::Button("今すぐ1個発生")) {
                    TriggerEmit(name);
                }
                ImGui::SameLine();
                if (ImGui::Button("削除")) {
                    nameToDestroy = name;
                }

                if (needReapply) {
                    instance->ApplyConfig(*managed);
                }
            }
            ImGui::PopID();
        }

        if (!nameToDestroy.empty()) {
            DestroyEmitter(nameToDestroy);
        }

        // --- 上書き確認ポップアップ ---
        if (instance->showOverwriteModal_) {
            ImGui::OpenPopup("GPUParticleManager 上書き確認");
            Logger::LogWarning("[GPUParticleManager] Overwrite check.");
            instance->showOverwriteModal_ = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("GPUParticleManager 上書き確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("該当フォルダには既に同名のファイルが存在します。上書きしますか？");
            ImGui::Text("フォルダパス : %s", instance->folderPath_.c_str());
            ImGui::Separator();

            if (ImGui::Button("上書きする", ImVec2(120, 0))) {
                instance->SaveToFileInternal(GetFullFilePath());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
                Logger::Log("[GPUParticleManager] Save cancelled.");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
#endif
    }
}
