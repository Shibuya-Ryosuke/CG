#include "ParticleCommon.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/Logger.h"
#include "../Shader/ShaderCompiler.h"
#include <cassert>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

#ifdef NRELEASE
#include <imgui.h>
#endif

namespace RyoEngine {

    namespace {
        // ルートシグネチャをシリアライズして作る共通処理
        Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc) {
            Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
            Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
            HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
            if (FAILED(hr)) {
                if (errorBlob) {
                    const std::string message = reinterpret_cast<char*>(errorBlob->GetBufferPointer());
                    Logger::LogError("[ParticleCommon] {}\n", message);
                }
                assert(false);
                return nullptr;
            }
            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
            hr = DirectXCommon::GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
            assert(SUCCEEDED(hr));
            return rootSignature;
        }

        D3D12_SHADER_BYTECODE ToBytecode(const Microsoft::WRL::ComPtr<IDxcBlob>& blob) {
            D3D12_SHADER_BYTECODE bytecode{};
            bytecode.pShaderBytecode = blob->GetBufferPointer();
            bytecode.BytecodeLength = blob->GetBufferSize();
            return bytecode;
        }
    }

    ParticleCommon* ParticleCommon::GetInstance() {
        static ParticleCommon instance;
        return &instance;
    }

    void ParticleCommon::Initialize() {
        ParticleCommon* instance = GetInstance();

        Logger::Log("ParticleCommon : Initializing...\n");
        instance->CreateComputeRootSignature();
        instance->CreateRenderRootSignature();
        if (!ReloadShaders()) {
            Logger::LogError("[ParticleCommon] シェーダーのコンパイルに失敗しました。直して保存すると自動で読み込み直します\n");
        }
        instance->RememberWriteTimes();
        Logger::LogSuccess("ParticleCommon : Initialized\n");
    }

    void ParticleCommon::Finalize() {
        ParticleCommon* instance = GetInstance();

        for (auto& pso : instance->renderPipelineStates_) {
            pso.Reset();
        }
        instance->renderRootSignature_.Reset();
        instance->simulatePipelineState_.Reset();
        instance->resetPipelineState_.Reset();
        instance->computeRootSignature_.Reset();
        instance->writeTimes_.clear();
        instance->dispatchCommands_.clear();
        instance->drawCommands_.clear();
    }

    void ParticleCommon::Dispatch() {
        for (const auto& command : GetInstance()->dispatchCommands_) {
            command();
        }
    }

    void ParticleCommon::Draw() {
        for (const auto& command : GetInstance()->drawCommands_) {
            command();
        }
    }

    bool ParticleCommon::IsReady() {
        ParticleCommon* instance = GetInstance();
        return instance->resetPipelineState_ && instance->simulatePipelineState_
            && instance->renderPipelineStates_[0] && instance->renderPipelineStates_[1];
    }

    void ParticleCommon::CreateComputeRootSignature() {
        D3D12_ROOT_PARAMETER rootParameters[6] = {};
        // 0: パーティクル本体 (u0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        // 1: カウンター (u1) [0]配った数 [1][2]分裂の子の数 [3]生きている数
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        // 2: 発生の命令 (t0)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[2].Descriptor.ShaderRegister = 0;
        // 3: シミュレーション用パラメータ (b0)
        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[3].Descriptor.ShaderRegister = 0;
        // 4: 分裂の子のリスト (u2)
        rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[4].Descriptor.ShaderRegister = 2;
        // 5: 発生のさせ方 (b1)
        rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[5].Descriptor.ShaderRegister = 1;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        desc.pParameters = rootParameters;
        desc.NumParameters = _countof(rootParameters);

        computeRootSignature_ = CreateRootSignature(desc);
    }

    void ParticleCommon::CreateRenderRootSignature() {
        D3D12_DESCRIPTOR_RANGE textureRange[1] = {};
        textureRange[0].BaseShaderRegister = 0;
        textureRange[0].NumDescriptors = 1;
        textureRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_PARAMETER rootParameters[3] = {};
        // 0: パーティクル本体 (Vertexのt0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        // 1: 描画用パラメータ (b0。Vertex/Pixelの両方で使う)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[1].Descriptor.ShaderRegister = 0;
        // 2: テクスチャ (Pixelのt0)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].DescriptorTable.pDescriptorRanges = textureRange;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(textureRange);

        D3D12_STATIC_SAMPLER_DESC staticSampler{};
        staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        staticSampler.ShaderRegister = 0;
        staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC desc{};
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE; // 頂点バッファは使わない (SV_VertexIDで板を作る)
        desc.pParameters = rootParameters;
        desc.NumParameters = _countof(rootParameters);
        desc.pStaticSamplers = &staticSampler;
        desc.NumStaticSamplers = 1;

        renderRootSignature_ = CreateRootSignature(desc);
    }

    bool ParticleCommon::CompileFile(const std::string& fileName, const wchar_t* profile,
        Microsoft::WRL::ComPtr<IDxcBlob>& outBlob, std::string& outError) {
        const std::string fullPath = shaderFolder_ + fileName;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            outError = "シェーダーファイルを開けませんでした: " + fullPath;
            return false;
        }
        std::stringstream stream;
        stream << file.rdbuf();
        const std::string source = stream.str();

        const std::vector<std::wstring> includeDirs = { Logger::ConvertString(shaderFolder_) };
        std::string error;
        outBlob = ShaderCompiler::CompileSource(source, Logger::ConvertString(fullPath), profile, includeDirs, error);
        if (!outBlob) {
            outError = fullPath + "\n" + error;
            return false;
        }
        return true;
    }

    bool ParticleCommon::ReloadShaders() {
        ParticleCommon* instance = GetInstance();
        ID3D12Device* device = DirectXCommon::GetDevice();

        // --- 全部コンパイルできてから入れ替える (途中で失敗したら前のPSOのまま) ---
        Microsoft::WRL::ComPtr<IDxcBlob> resetBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> simulateBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> vertexBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> pixelBlob;
        std::string error;
        bool succeeded = instance->CompileFile(instance->resetShaderFile_, L"cs_6_0", resetBlob, error)
            && instance->CompileFile(instance->simulateShaderFile_, L"cs_6_0", simulateBlob, error)
            && instance->CompileFile(instance->vertexShaderFile_, L"vs_6_0", vertexBlob, error)
            && instance->CompileFile(instance->pixelShaderFile_, L"ps_6_0", pixelBlob, error);
        if (!succeeded) {
            instance->lastError_ = error;
            Logger::LogError("[ParticleCommon] {}\n", error);
            return false;
        }

        // --- コンピュート ---
        Microsoft::WRL::ComPtr<ID3D12PipelineState> newReset;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> newSimulate;
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = instance->computeRootSignature_.Get();
            desc.CS = ToBytecode(resetBlob);
            HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newReset));
            if (FAILED(hr)) {
                instance->lastError_ = "リセット用のPSOを作れませんでした";
                Logger::LogError("[ParticleCommon] {}\n", instance->lastError_);
                return false;
            }
            desc.CS = ToBytecode(simulateBlob);
            hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newSimulate));
            if (FAILED(hr)) {
                instance->lastError_ = "シミュレーション用のPSOを作れませんでした";
                Logger::LogError("[ParticleCommon] {}\n", instance->lastError_);
                return false;
            }
        }

        // --- 描画 ---
        Microsoft::WRL::ComPtr<ID3D12PipelineState> newRender[2];
        {
            D3D12_RASTERIZER_DESC rasterizerDesc{};
            rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 板ポリなので裏表どちらも描く
            rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

            D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
            depthStencilDesc.DepthEnable = true;
            depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 半透明なので深度は書かない
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

            D3D12_BLEND_DESC alphaBlendDesc{};
            alphaBlendDesc.RenderTarget[0].BlendEnable = true;
            alphaBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            alphaBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            alphaBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
            alphaBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
            alphaBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
            alphaBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            alphaBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            D3D12_BLEND_DESC additiveBlendDesc = alphaBlendDesc;
            additiveBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = instance->renderRootSignature_.Get();
            desc.VS = ToBytecode(vertexBlob);
            desc.PS = ToBytecode(pixelBlob);
            desc.RasterizerState = rasterizerDesc;
            desc.DepthStencilState = depthStencilDesc;
            desc.NumRenderTargets = 1;
            desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // PostProcessのHDRシーンバッファに合わせる
            desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;      // メインの深度バッファに合わせる
            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            desc.SampleDesc.Count = 1;
            desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

            for (uint32_t i = 0; i < 2; ++i) {
                desc.BlendState = (i == 1) ? additiveBlendDesc : alphaBlendDesc;
                HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newRender[i]));
                if (FAILED(hr)) {
                    instance->lastError_ = "描画用のPSOを作れませんでした";
                    Logger::LogError("[ParticleCommon] {}\n", instance->lastError_);
                    return false;
                }
            }
        }

        // NOTE: 前のフレームのGPU処理はNewFrameの時点で完了しているので、ここで入れ替えて問題ない
        instance->resetPipelineState_ = newReset;
        instance->simulatePipelineState_ = newSimulate;
        instance->renderPipelineStates_[0] = newRender[0];
        instance->renderPipelineStates_[1] = newRender[1];
        instance->lastError_.clear();
        ++instance->reloadCount_;

        Logger::LogSuccess("[ParticleCommon] シェーダーを読み込みました\n");
        return true;
    }

    void ParticleCommon::RememberWriteTimes() {
        writeTimes_.clear();
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(shaderFolder_, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            writeTimes_[entry.path().generic_string()] = entry.last_write_time(ec);
        }
    }

    bool ParticleCommon::HasShaderFileChanged() {
        std::error_code ec;
        size_t fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(shaderFolder_, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            ++fileCount;
            auto it = writeTimes_.find(entry.path().generic_string());
            if (it == writeTimes_.end() || it->second != entry.last_write_time(ec)) {
                return true;
            }
        }
        return fileCount != writeTimes_.size();
    }

    void ParticleCommon::CheckHotReload(float deltaTime) {
#ifdef NRELEASE
        ParticleCommon* instance = GetInstance();
        if (!instance->autoReload_) {
            return;
        }
        instance->watchTimer_ -= deltaTime;
        if (instance->watchTimer_ > 0.0f) {
            return;
        }
        instance->watchTimer_ = instance->watchInterval_;

        if (instance->HasShaderFileChanged()) {
            // 保存の途中で読むと壊れたファイルになることがあるので、日時は先に覚え直しておく
            // (失敗しても、もう一度保存すれば再挑戦される)
            instance->RememberWriteTimes();
            ReloadShaders();
        }
#else
        (void)deltaTime;
#endif
    }

    void ParticleCommon::DrawImGui() {
#ifdef NRELEASE
        ParticleCommon* instance = GetInstance();

        ImGui::Text("シェーダー : %s", instance->shaderFolder_.c_str());
        if (ImGui::Button("シェーダーを再読み込み")) {
            instance->RememberWriteTimes();
            ReloadShaders();
        }
        ImGui::SameLine();
        ImGui::Checkbox("保存したら自動で再読み込み##shader", &instance->autoReload_);
        ImGui::Text("読み込んだ回数 : %u", instance->reloadCount_);
        if (!instance->lastError_.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "エラー (前のシェーダーのまま動いています)");
            ImGui::TextWrapped("%s", instance->lastError_.c_str());
        }
#endif
    }
}
