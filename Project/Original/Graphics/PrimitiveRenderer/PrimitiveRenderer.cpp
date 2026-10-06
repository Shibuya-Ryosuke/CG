#include "PrimitiveRenderer.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../3D/ModelCommon.h"
#include <cmath>
#include <cstring>
#include <cassert>

// 使用するシェーダー: Resources/HLSL/Primitive/Primitive_VS.hlsl, Primitive_PS.hlsl, Primitive.hlsli
// (ModelCommon.cppの ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model.VS.hlsl", ...) と
//  同じ呼び方に合わせて "Resources/EngineResources/HLSL/Primitive/Primitive_VS.hlsl" を読みに行っている。
//  実際の配置フォルダがHLSL/Model/以外の場所であれば、CreatePipelineStates()内のパスを合わせて変更すること)

namespace RyoEngine {

    namespace {
        constexpr uint32_t kMaxVerticesPerGroup = 16384; // 1グループ(線3D/面3D/線2D/面2D)あたりの最大頂点数
        constexpr float kDefaultScreenWidth = 1280.0f;
        constexpr float kDefaultScreenHeight = 720.0f;
    }

    // ---- static メンバの実体 ----
    std::vector<PrimitiveRenderer::PrimitiveVertex> PrimitiveRenderer::lineVertices3D_;
    std::vector<PrimitiveRenderer::PrimitiveVertex> PrimitiveRenderer::triVertices3D_;
    std::vector<PrimitiveRenderer::PrimitiveVertex> PrimitiveRenderer::lineVertices2D_;
    std::vector<PrimitiveRenderer::PrimitiveVertex> PrimitiveRenderer::triVertices2D_;

    Matrix4x4 PrimitiveRenderer::viewProjectionMatrix_ = MakeIdentity4x4();
    Matrix4x4 PrimitiveRenderer::orthographicMatrix_ = MakeIdentity4x4();
    float PrimitiveRenderer::screenWidth_ = kDefaultScreenWidth;
    float PrimitiveRenderer::screenHeight_ = kDefaultScreenHeight;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> PrimitiveRenderer::rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PrimitiveRenderer::linePSO_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> PrimitiveRenderer::trianglePSO_;

    Microsoft::WRL::ComPtr<ID3D12Resource> PrimitiveRenderer::vertexResource_;
    PrimitiveRenderer::PrimitiveVertex* PrimitiveRenderer::mappedVertexData_ = nullptr;
    uint32_t PrimitiveRenderer::vertexCapacity_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> PrimitiveRenderer::vpResource3D_;
    Matrix4x4* PrimitiveRenderer::vpData3D_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> PrimitiveRenderer::vpResource2D_;
    Matrix4x4* PrimitiveRenderer::vpData2D_ = nullptr;

    // ============================================================
    // 初期化 / 終了
    // ============================================================

    void PrimitiveRenderer::Initialize() {
        auto device = DirectXCommon::GetInstance()->GetDevice();

        // --- 動的頂点バッファ(4グループぶんを1本の大きなUpload Heapとして確保) ---
        vertexCapacity_ = kMaxVerticesPerGroup * 4;
        vertexResource_ = DirectXCommon::CreateBufferResource(device, sizeof(PrimitiveVertex) * vertexCapacity_);
        vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData_));

        // --- カメラ用定数バッファ(3D/2D) ---
        vpResource3D_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        vpResource3D_->Map(0, nullptr, reinterpret_cast<void**>(&vpData3D_));
        *vpData3D_ = MakeIdentity4x4();

        vpResource2D_ = DirectXCommon::CreateBufferResource(device, sizeof(Matrix4x4));
        vpResource2D_->Map(0, nullptr, reinterpret_cast<void**>(&vpData2D_));
        *vpData2D_ = MakeIdentity4x4();

        SetScreenSize(kDefaultScreenWidth, kDefaultScreenHeight);

        // --- ルートシグネチャ & PSO ---
        CreateRootSignature();
        CreatePipelineStates();
    }

    void PrimitiveRenderer::CreateRootSignature() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // b0 = ViewProjection行列のみ(Material/Texture/Lightは使わない)
        D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
        descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        D3D12_ROOT_PARAMETER rootParameters[1] = {};
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Descriptor.ShaderRegister = 0;

        descriptionRootSignature.pParameters = rootParameters;
        descriptionRootSignature.NumParameters = _countof(rootParameters);
        // テクスチャ/サンプラーを使わないので DescriptorRange・StaticSampler は無し

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
        hr = D3D12SerializeRootSignature(&descriptionRootSignature,
            D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        assert(SUCCEEDED(hr));

        hr = device->CreateRootSignature(0,
            signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
        assert(SUCCEEDED(hr));
    }

    void PrimitiveRenderer::CreatePipelineStates() {
        auto device = DirectXCommon::GetInstance()->GetDevice();
        HRESULT hr = S_OK;

        // InputLayout: POSITION(float4) / COLOR(float4)
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
        inputElementDescs[0].SemanticName = "POSITION";
        inputElementDescs[0].SemanticIndex = 0;
        inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

        inputElementDescs[1].SemanticName = "COLOR";
        inputElementDescs[1].SemanticIndex = 0;
        inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.pInputElementDescs = inputElementDescs;
        inputLayoutDesc.NumElements = _countof(inputElementDescs);

        // BlendState: 半透明(アルファブレンド)対応。ModelCommonの通常描画と同じ設定。
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

        // RasterizerState: 当たり判定の可視化用なので、裏面カリングはしない(裏側からも見えるように)
        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

        // DepthStencilState: 実オブジェクトには隠れてほしいのでDepthEnable=trueのままにするが、
        // 半透明の重ね描き同士でZ Fightingしないよう、深度の書き込みだけはしない。
        // (壁越しにも当たり判定を見せたい場合は DepthFunc を D3D12_COMPARISON_FUNC_ALWAYS に変更する)
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob =
            ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Primitive/Primitive_VS.hlsl", L"vs_6_0");
        assert(vertexShaderBlob != nullptr);

        Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob =
            ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Primitive/Primitive_PS.hlsl", L"ps_6_0");
        assert(pixelShaderBlob != nullptr);

        D3D12_GRAPHICS_PIPELINE_STATE_DESC baseDesc{};
        baseDesc.pRootSignature = rootSignature_.Get();
        baseDesc.InputLayout = inputLayoutDesc;
        baseDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        baseDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        baseDesc.BlendState = blendDesc;
        baseDesc.RasterizerState = rasterizerDesc;
        baseDesc.NumRenderTargets = 1;
        baseDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        baseDesc.SampleDesc.Count = 1;
        baseDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
        baseDesc.DepthStencilState = depthStencilDesc;
        baseDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        // 三角形(塗りつぶし)用PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC triangleDesc = baseDesc;
        triangleDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        hr = device->CreateGraphicsPipelineState(&triangleDesc, IID_PPV_ARGS(&trianglePSO_));
        assert(SUCCEEDED(hr));

        // 線(ワイヤーフレーム)用PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC lineDesc = baseDesc;
        lineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        hr = device->CreateGraphicsPipelineState(&lineDesc, IID_PPV_ARGS(&linePSO_));
        assert(SUCCEEDED(hr));
    }

    void PrimitiveRenderer::Finalize() {
        if (vertexResource_) { vertexResource_->Unmap(0, nullptr); vertexResource_.Reset(); }
        if (vpResource3D_) { vpResource3D_->Unmap(0, nullptr); vpResource3D_.Reset(); }
        if (vpResource2D_) { vpResource2D_->Unmap(0, nullptr); vpResource2D_.Reset(); }
        mappedVertexData_ = nullptr;
        vpData3D_ = nullptr;
        vpData2D_ = nullptr;
        rootSignature_.Reset();
        linePSO_.Reset();
        trianglePSO_.Reset();
    }

    void PrimitiveRenderer::NewFrame() {
        lineVertices3D_.clear();
        triVertices3D_.clear();
        lineVertices2D_.clear();
        triVertices2D_.clear();
    }

    void PrimitiveRenderer::SetCamera(const Camera& camera) {
        viewProjectionMatrix_ = camera.GetViewProjectionMatrix();
        *vpData3D_ = viewProjectionMatrix_;
    }

    void PrimitiveRenderer::SetScreenSize(float width, float height) {
        screenWidth_ = width;
        screenHeight_ = height;
        // 左上原点・ピクセル単位のスクリーン座標をそのままNDCへ変換する正射影行列
        orthographicMatrix_ = MakeOrthographicMatrix(0.0f, 0.0f, width, height, 0.0f, 100.0f);
        if (vpData2D_) {
            *vpData2D_ = orthographicMatrix_;
        }
    }

    // ============================================================
    // Flush (GPUへの送信)
    // ============================================================

    void PrimitiveRenderer::Flush() {
        if (!mappedVertexData_) return; // Initialize()未実行

        // 4グループそれぞれの書き込み先頭(頂点単位のオフセット)
        constexpr  uint32_t offsetLine3D = kMaxVerticesPerGroup * 0;
        constexpr  uint32_t offsetTri3D = kMaxVerticesPerGroup * 1;
        constexpr  uint32_t offsetLine2D = kMaxVerticesPerGroup * 2;
        constexpr  uint32_t offsetTri2D = kMaxVerticesPerGroup * 3;

        auto CopyGroup = [](const std::vector<PrimitiveVertex>& src, uint32_t dstOffsetVertices) {
            uint32_t count = static_cast<uint32_t>(src.size());
            if (count > kMaxVerticesPerGroup) {
                // NOTE: 1フレームに描きすぎ。kMaxVerticesPerGroupを増やすか、描画数を減らしてください。
                count = kMaxVerticesPerGroup;
            }
            if (count > 0) {
                std::memcpy(mappedVertexData_ + dstOffsetVertices, src.data(), sizeof(PrimitiveVertex) * count);
            }
            return count;
            };

        const uint32_t countLine3D = CopyGroup(lineVertices3D_, offsetLine3D);
        const uint32_t countTri3D = CopyGroup(triVertices3D_, offsetTri3D);
        const uint32_t countLine2D = CopyGroup(lineVertices2D_, offsetLine2D);
        const uint32_t countTri2D = CopyGroup(triVertices2D_, offsetTri2D);

        D3D12_GPU_VIRTUAL_ADDRESS vbBase = vertexResource_->GetGPUVirtualAddress();
        D3D12_GPU_VIRTUAL_ADDRESS vp3DAddress = vpResource3D_->GetGPUVirtualAddress();
        D3D12_GPU_VIRTUAL_ADDRESS vp2DAddress = vpResource2D_->GetGPUVirtualAddress();

        ModelCommon::GetInstance()->SetDrawCommands(
            [vbBase, vp3DAddress, vp2DAddress,
            countLine3D, countTri3D, countLine2D, countTri2D]() {
                auto commandList = DirectXCommon::GetInstance()->GetCommandList();

                // NOTE: ModelCommon::Draw() は BeginDraw() で ModelCommon 自身の RootSignature を
                //       1度セットしてから drawCommands_ を順に実行する。
                //       PrimitiveRenderer は別のRootSignature(b0=ViewProjectionのみ)を使うので、
                //       ここで明示的に自分のRootSignatureに切り替える。
                commandList->SetGraphicsRootSignature(rootSignature_.Get());

                auto DrawGroup = [&](ID3D12PipelineState* pso, D3D_PRIMITIVE_TOPOLOGY topology,
                    D3D12_GPU_VIRTUAL_ADDRESS vpAddress, uint32_t offsetVertices, uint32_t count) {
                        if (count == 0) return;

                        D3D12_VERTEX_BUFFER_VIEW vbv{};
                        vbv.BufferLocation = vbBase + static_cast<UINT64>(offsetVertices) * sizeof(PrimitiveVertex);
                        vbv.SizeInBytes = static_cast<UINT>(sizeof(PrimitiveVertex) * count);
                        vbv.StrideInBytes = sizeof(PrimitiveVertex);

                        commandList->SetPipelineState(pso);
                        commandList->IASetPrimitiveTopology(topology);
                        commandList->SetGraphicsRootConstantBufferView(0, vpAddress);
                        commandList->IASetVertexBuffers(0, 1, &vbv);
                        commandList->DrawInstanced(count, 1, 0, 0);
                    };

                DrawGroup(linePSO_.Get(), D3D_PRIMITIVE_TOPOLOGY_LINELIST, vp3DAddress, offsetLine3D, countLine3D);
                DrawGroup(trianglePSO_.Get(), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vp3DAddress, offsetTri3D, countTri3D);
                DrawGroup(linePSO_.Get(), D3D_PRIMITIVE_TOPOLOGY_LINELIST, vp2DAddress, offsetLine2D, countLine2D);
                DrawGroup(trianglePSO_.Get(), D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vp2DAddress, offsetTri2D, countTri2D);
            });
    }

    // ============================================================
    // 3D
    // ============================================================

    void PrimitiveRenderer::DrawSphere(const Vector3& center, float radius, uint32_t subdivision,
        const Vector4& color, PrimitiveDrawMode mode) {
        AppendSphere(center, radius, subdivision, color, mode);
    }

    void PrimitiveRenderer::AppendSphere(const Vector3& center, float radius, uint32_t subdivision,
        const Vector4& color, PrimitiveDrawMode mode) {
        if (subdivision < 2) subdivision = 2;

        const float kLatEvery = 3.1415926535f / static_cast<float>(subdivision);
        const float kLonEvery = (3.1415926535f * 2.0f) / static_cast<float>(subdivision);

        auto SpherePoint = [&](float lat, float lon) -> Vector4 {
            Vector3 p = {
                center.x + radius * std::cosf(lat) * std::cosf(lon),
                center.y + radius * std::sinf(lat),
                center.z + radius * std::cosf(lat) * std::sinf(lon)
            };
            return { p.x, p.y, p.z, 1.0f };
            };

        for (uint32_t latIndex = 0; latIndex < subdivision; ++latIndex) {
            float lat = -3.1415926535f / 2.0f + latIndex * kLatEvery;
            float nextLat = lat + kLatEvery;

            for (uint32_t lonIndex = 0; lonIndex < subdivision; ++lonIndex) {
                float lon = lonIndex * kLonEvery;
                float nextLon = lon + kLonEvery;

                Vector4 a = SpherePoint(lat, lon);
                Vector4 b = SpherePoint(nextLat, lon);
                Vector4 c = SpherePoint(lat, nextLon);
                Vector4 d = SpherePoint(nextLat, nextLon);

                if (mode == PrimitiveDrawMode::Wireframe) {
                    lineVertices3D_.push_back({ a, color });
                    lineVertices3D_.push_back({ b, color });
                    lineVertices3D_.push_back({ a, color });
                    lineVertices3D_.push_back({ c, color });
                } else {
                    triVertices3D_.push_back({ a, color });
                    triVertices3D_.push_back({ b, color });
                    triVertices3D_.push_back({ c, color });

                    triVertices3D_.push_back({ c, color });
                    triVertices3D_.push_back({ b, color });
                    triVertices3D_.push_back({ d, color });
                }
            }
        }
    }

    void PrimitiveRenderer::DrawBox(const Vector3& center, const Vector3& rotate, const Vector3& size,
        const Vector4& color, PrimitiveDrawMode mode) {
        Vector3 halfSize = { size.x * 0.5f, size.y * 0.5f, size.z * 0.5f };
        Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate, center);
        AppendBox(worldMatrix, halfSize, color, mode);
    }

    void PrimitiveRenderer::DrawAABB(const AABB& aabb, const Vector4& color, PrimitiveDrawMode mode) {
        Vector3 center = (aabb.min + aabb.max) * 0.5f;
        Vector3 halfSize = (aabb.max - aabb.min) * 0.5f;
        Matrix4x4 worldMatrix = MakeTranslateMatrix(center);
        AppendBox(worldMatrix, halfSize, color, mode);
    }

    void PrimitiveRenderer::DrawOBB(const OBB& obb, const Vector4& color, PrimitiveDrawMode mode) {
        Matrix4x4 worldMatrix = CreateWorldMatrixFromOBB(obb);
        AppendBox(worldMatrix, obb.size, color, mode);
    }

    void PrimitiveRenderer::AppendBox(const Matrix4x4& worldMatrix, const Vector3& halfSize,
        const Vector4& color, PrimitiveDrawMode mode) {
        Vector3 local[8] = {
            { -halfSize.x, -halfSize.y, -halfSize.z },
            {  halfSize.x, -halfSize.y, -halfSize.z },
            { -halfSize.x,  halfSize.y, -halfSize.z },
            {  halfSize.x,  halfSize.y, -halfSize.z },
            { -halfSize.x, -halfSize.y,  halfSize.z },
            {  halfSize.x, -halfSize.y,  halfSize.z },
            { -halfSize.x,  halfSize.y,  halfSize.z },
            {  halfSize.x,  halfSize.y,  halfSize.z },
        };

        Vector4 world[8];
        for (int i = 0; i < 8; ++i) {
            Vector3 p = TransformVector3(local[i], worldMatrix);
            world[i] = { p.x, p.y, p.z, 1.0f };
        }

        if (mode == PrimitiveDrawMode::Wireframe) {
            static const int kEdges[12][2] = {
                {0,1},{1,3},{3,2},{2,0}, // 手前の面
                {4,5},{5,7},{7,6},{6,4}, // 奥の面
                {0,4},{1,5},{2,6},{3,7}, // 縦
            };
            for (auto& e : kEdges) {
                lineVertices3D_.push_back({ world[e[0]], color });
                lineVertices3D_.push_back({ world[e[1]], color });
            }
        } else {
            static const int kFaces[12][3] = {
                {0,1,3},{0,3,2}, // 手前(-z)
                {5,4,6},{5,6,7}, // 奥(+z)
                {4,0,2},{4,2,6}, // 左(-x)
                {1,5,7},{1,7,3}, // 右(+x)
                {2,3,7},{2,7,6}, // 上(+y)
                {4,5,1},{4,1,0}, // 下(-y)
            };
            for (auto& f : kFaces) {
                triVertices3D_.push_back({ world[f[0]], color });
                triVertices3D_.push_back({ world[f[1]], color });
                triVertices3D_.push_back({ world[f[2]], color });
            }
        }
    }

    void PrimitiveRenderer::DrawLine3D(const Vector3& start, const Vector3& end, const Vector4& color) {
        lineVertices3D_.push_back({ { start.x, start.y, start.z, 1.0f }, color });
        lineVertices3D_.push_back({ { end.x, end.y, end.z, 1.0f }, color });
    }

    // ============================================================
    // 2D
    // ============================================================

    void PrimitiveRenderer::DrawRect2D(const Vector2& center, const Vector2& size, float rotate,
        const Vector4& color, PrimitiveDrawMode mode) {
        Vector2 half = { size.x * 0.5f, size.y * 0.5f };
        Matrix3x3 worldMatrix = MakeAffineMatrix(Vector2{ 1.0f, 1.0f }, rotate, center);

        Vector2 local[4] = {
            { -half.x, -half.y },
            {  half.x, -half.y },
            { -half.x,  half.y },
            {  half.x,  half.y },
        };
        Vector4 world[4];
        for (int i = 0; i < 4; ++i) {
            Vector2 p = TransformVector2(local[i], worldMatrix);
            world[i] = { p.x, p.y, 0.0f, 1.0f };
        }

        if (mode == PrimitiveDrawMode::Wireframe) {
            static const int kEdges[4][2] = { {0,1},{1,3},{3,2},{2,0} };
            for (auto& e : kEdges) {
                lineVertices2D_.push_back({ world[e[0]], color });
                lineVertices2D_.push_back({ world[e[1]], color });
            }
        } else {
            triVertices2D_.push_back({ world[0], color });
            triVertices2D_.push_back({ world[1], color });
            triVertices2D_.push_back({ world[3], color });

            triVertices2D_.push_back({ world[0], color });
            triVertices2D_.push_back({ world[3], color });
            triVertices2D_.push_back({ world[2], color });
        }
    }

    void PrimitiveRenderer::DrawCircle2D(const Vector2& center, float radius, uint32_t subdivision,
        const Vector4& color, PrimitiveDrawMode mode) {
        if (subdivision < 3) subdivision = 3;
        const float kEvery = (3.1415926535f * 2.0f) / static_cast<float>(subdivision);

        Vector4 centerV = { center.x, center.y, 0.0f, 1.0f };

        for (uint32_t i = 0; i < subdivision; ++i) {
            float angle = i * kEvery;
            float nextAngle = (i + 1) * kEvery;

            Vector4 a = { center.x + std::cosf(angle) * radius, center.y + std::sinf(angle) * radius, 0.0f, 1.0f };
            Vector4 b = { center.x + std::cosf(nextAngle) * radius, center.y + std::sinf(nextAngle) * radius, 0.0f, 1.0f };

            if (mode == PrimitiveDrawMode::Wireframe) {
                lineVertices2D_.push_back({ a, color });
                lineVertices2D_.push_back({ b, color });
            } else {
                triVertices2D_.push_back({ centerV, color });
                triVertices2D_.push_back({ a, color });
                triVertices2D_.push_back({ b, color });
            }
        }
    }

    void PrimitiveRenderer::DrawLine2D(const Vector2& start, const Vector2& end, const Vector4& color) {
        lineVertices2D_.push_back({ { start.x, start.y, 0.0f, 1.0f }, color });
        lineVertices2D_.push_back({ { end.x, end.y, 0.0f, 1.0f }, color });
    }
}
