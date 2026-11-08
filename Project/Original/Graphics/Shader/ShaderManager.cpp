#include "ShaderManager.h"
#include "../../Core/Base/Logger.h"
#include "ShaderCompiler.h"
#include "../../Core/Asset/AssetRegistry.h"
#include "../2D/TextureManager.h"
#include <cassert>
#include <filesystem>
#include <system_error>
#ifdef NRELEASE
#include <imgui.h>
#endif

namespace RyoEngine {

    ShaderManager* ShaderManager::GetInstance() {
        static ShaderManager instance;
        return &instance;
    }

    void ShaderManager::Initialize() {
        ShaderManager* instance = GetInstance();

        Logger::Log("ShaderManager : Initializing...\n");
        instance->shaders_.clear();
        instance->pathToHandle_.clear();
        instance->reloadCount_ = 0;

        // 0番: エンジン標準 (DefaultSurface)
        Entry defaultEntry;
        defaultEntry.name = "(エンジン標準)";
        instance->shaders_.push_back(std::move(defaultEntry));
        instance->Compile(instance->shaders_[kDefaultShader]);

        Logger::LogSuccess("ShaderManager : Initialized\n");
    }

    void ShaderManager::Finalize() {
        ShaderManager* instance = GetInstance();

        Logger::Log("ShaderManager : Finalizing...\n");
        instance->shaders_.clear();
        instance->pathToHandle_.clear();
        Logger::LogSuccess("ShaderManager : Finalized\n");
    }

    // ------------------------------------------------------------------
    // 読み込み
    // ------------------------------------------------------------------
    uint32_t ShaderManager::Load(const std::string& nameOrPath) {
        return GetInstance()->LoadImpl(Type::Model, nameOrPath);
    }

    uint32_t ShaderManager::LoadPostEffect(const std::string& nameOrPath) {
        return GetInstance()->LoadImpl(Type::PostEffect, nameOrPath);
    }

    uint32_t ShaderManager::LoadImpl(Type type, const std::string& nameOrPath) {
        const std::string filePath = AssetRegistry::Resolve(AssetRegistry::Type::Shader, nameOrPath);
        if (filePath.empty()) {
            return kDefaultShader; // 見つからない (警告は AssetRegistry が出している)
        }
        // 同じファイルでも、種類が違えば別のシェーダーとして扱う
        const std::string key = (type == Type::Model ? "Model:" : "PostEffect:") + filePath;
        if (auto it = pathToHandle_.find(key); it != pathToHandle_.end()) {
            return it->second;
        }

        Entry entry;
        entry.type = type;
        entry.name = Logger::ConvertString(std::filesystem::path(Logger::ConvertString(filePath)).stem().wstring()); // 表示用 (拡張子なしのファイル名)
        entry.filePath = filePath;
        Compile(entry);

        const uint32_t handle = static_cast<uint32_t>(shaders_.size());
        shaders_.push_back(std::move(entry));
        pathToHandle_[key] = handle;
        return handle;
    }

    void ShaderManager::Compile(Entry& entry) {
        if (entry.type == Type::PostEffect) {
            CompilePostEffect(entry);
            return;
        }

        // アプリのシェーダー (無ければエンジン標準の DefaultSurface) を #include する
        const std::string userFile = entry.filePath.empty() ? "Model/DefaultSurface.hlsli" : ToIncludePath(entry.filePath);
        const char* common = "Model/ModelSurface.hlsli";

        ModelProgram program;
        std::string error;
        const bool succeeded =
            CompileSource(userFile, common, "MODEL_STAGE_VS", "Model/ModelMain.VS.hlsli", L"vs_6_0", program.vertexShader, error) &&
            CompileSource(userFile, common, "MODEL_STAGE_PS", "Model/ModelMain.PS.hlsli", L"ps_6_0", program.pixelShader, error) &&
            CompileSource(userFile, common, "MODEL_STAGE_SHADOW", "Model/ModelMainShadow.VS.hlsli", L"vs_6_0", program.shadowVertexShader, error);

        if (succeeded) {
            entry.program = std::move(program);
            entry.failed = false;
            entry.error.clear();
            Logger::LogSuccess("[ShaderManager] コンパイル成功: {}\n", entry.name);
            return;
        }

        // エンジン標準が壊れているときは描画できないので止める
        if (entry.filePath.empty()) {
            Logger::LogError("[ShaderManager] エンジン標準のシェーダーのコンパイルに失敗しました\n{}\n", error);
            assert(false);
            return;
        }

        // アプリのシェーダーの失敗は止めずに、エンジン標準で代わりに描く
        entry.failed = true;
        entry.error = error;
        if (!shaders_.empty()) {
            entry.program = shaders_[kDefaultShader].program;
        }
        Logger::LogError("[ShaderManager] コンパイル失敗 (エンジン標準で代わりに描きます): {}\n{}\n", entry.filePath, error);
    }

    void ShaderManager::CompilePostEffect(Entry& entry) {
        std::vector<uint8_t> pixelShader;
        std::string error;
        const bool succeeded = CompileSource(ToIncludePath(entry.filePath), "PostEffect/PostEffectCommon.hlsli",
            "POST_EFFECT_STAGE_PS", "PostEffect/PostEffectMain.PS.hlsli", L"ps_6_0", pixelShader, error);

        if (succeeded) {
            entry.postEffectPixelShader = std::move(pixelShader);
            entry.failed = false;
            entry.error.clear();
            Logger::LogSuccess("[ShaderManager] コンパイル成功 (ポストエフェクト): {}\n", entry.name);
            return;
        }

        // 失敗したポストエフェクトは止めずに飛ばす (その効果だけかからない)
        entry.postEffectPixelShader.clear();
        entry.failed = true;
        entry.error = error;
        Logger::LogError("[ShaderManager] コンパイル失敗 (このポストエフェクトは飛ばします): {}\n{}\n", entry.filePath, error);
    }

    bool ShaderManager::CompileSource(const std::string& userFile, const char* commonFile, const char* stageDefine, const char* mainFile,
        const wchar_t* profile, std::vector<uint8_t>& outBytecode, std::string& outError) {
        // 型紙をまとめた1つのソースを作る (ModelSurface.hlsli / PostEffectCommon.hlsli の先頭のコメント参照)
        std::string source;
        source += "#define " + std::string(stageDefine) + "\n";
        source += "#include \"" + std::string(commonFile) + "\"\n";
        source += "#include \"" + userFile + "\"\n";
        source += "#include \"" + std::string(mainFile) + "\"\n";

        // DXCはこの名前をファイル名として扱うので、英数字だけにする (日本語・空白・カッコが入ると読み込みに失敗する)。
        // アプリのシェーダーのエラーは、#include したファイルのパスと行番号で表示される
        const std::wstring sourceName = L"Shader_" + Logger::ConvertString(std::string(stageDefine)) + L".hlsl";
        std::string error;
        Microsoft::WRL::ComPtr<IDxcBlob> blob = ShaderCompiler::CompileSource(source, sourceName, profile, { engineShaderDirectory_ }, error);
        if (!blob) {
            outError = std::string(stageDefine) + "\n" + error;
            return false;
        }

        const uint8_t* data = static_cast<const uint8_t*>(blob->GetBufferPointer());
        outBytecode.assign(data, data + blob->GetBufferSize());
        return true;
    }

    std::string ShaderManager::ToIncludePath(const std::string& filePath) {
        std::error_code ec;
        const std::filesystem::path absolute = std::filesystem::absolute(std::filesystem::path(Logger::ConvertString(filePath)), ec);
        return Logger::ConvertString(absolute.generic_wstring());
    }

    // ------------------------------------------------------------------
    // テクスチャ・取得
    // ------------------------------------------------------------------
    void ShaderManager::SetTexture(uint32_t shader, uint32_t slot, uint32_t textureHandle) {
        ShaderManager* instance = GetInstance();
        if (shader >= instance->shaders_.size() || slot >= kTextureSlotCount) {
            Logger::LogWarning("[ShaderManager] SetTexture: 無効なシェーダーかスロットです (shader={}, slot={})\n", shader, slot);
            return;
        }
        instance->shaders_[shader].textures[slot] = textureHandle;
    }

    uint32_t ShaderManager::GetTexture(uint32_t shader, uint32_t slot) {
        ShaderManager* instance = GetInstance();
        if (shader >= instance->shaders_.size() || slot >= kTextureSlotCount) {
            return TextureManager::GetWhiteTex();
        }
        const uint32_t handle = instance->shaders_[shader].textures[slot];
        return handle != 0 ? handle : TextureManager::GetWhiteTex();
    }

    const ShaderManager::ModelProgram& ShaderManager::GetModelProgram(uint32_t shader) {
        ShaderManager* instance = GetInstance();
        // 無効なハンドル・ポストエフェクト用のハンドルなら、エンジン標準
        if (shader >= instance->shaders_.size() || instance->shaders_[shader].type != Type::Model) {
            shader = kDefaultShader;
        }
        return instance->shaders_[shader].program;
    }

    const std::vector<uint8_t>* ShaderManager::GetPostEffectPixelShader(uint32_t shader) {
        ShaderManager* instance = GetInstance();
        if (shader >= instance->shaders_.size()) {
            return nullptr;
        }
        const Entry& entry = instance->shaders_[shader];
        if (entry.type != Type::PostEffect || entry.failed || entry.postEffectPixelShader.empty()) {
            return nullptr;
        }
        return &entry.postEffectPixelShader;
    }

    const std::string& ShaderManager::GetName(uint32_t shader) {
        static const std::string empty;
        ShaderManager* instance = GetInstance();
        return shader < instance->shaders_.size() ? instance->shaders_[shader].name : empty;
    }

    // ------------------------------------------------------------------
    // ホットリロード
    // ------------------------------------------------------------------
    void ShaderManager::ReloadAll() {
        ShaderManager* instance = GetInstance();

        Logger::Log("[ShaderManager] シェーダーを再読み込みします\n");
        // エンジン標準を先に (アプリのシェーダーが失敗したときの代わりに使うため)
        for (Entry& entry : instance->shaders_) {
            instance->Compile(entry);
        }
        ++instance->reloadCount_;
    }

    void ShaderManager::DrawImGui() {
#ifdef NRELEASE
        ShaderManager* instance = GetInstance();

        ImGui::Begin("Shader");
        if (ImGui::Button("シェーダーを再読み込み")) {
            ReloadAll();
        }
        ImGui::Separator();
        for (size_t i = 0; i < instance->shaders_.size(); ++i) {
            const Entry& entry = instance->shaders_[i];
            ImGui::PushID(static_cast<int>(i));
            const char* typeName = (entry.type == Type::PostEffect) ? "ポストエフェクト" : "モデル";
            if (entry.failed) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[%zu] %s (%s) : エラー (%s)", i, entry.name.c_str(), typeName,
                    entry.type == Type::PostEffect ? "この効果は飛ばしています" : "標準で描画中");
                ImGui::TextWrapped("%s", entry.error.c_str());
            } else {
                ImGui::Text("[%zu] %s (%s)", i, entry.name.c_str(), typeName);
            }
            ImGui::PopID();
        }
        ImGui::End();
#endif
    }
}
