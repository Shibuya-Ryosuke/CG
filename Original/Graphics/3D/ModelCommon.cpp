#include "Model.h"
#include "../../Core/Base/Logger.h"
#include "../../Core/Base/DirectXCommon.h"
#include "../../Core/Base/ShaderCompiler.h"
#include "../../Graphics/2D/TextureManager.h"
#include "../Shadow/ShadowMap.h"
#include "ModelCommon.h"
#include <cassert>
#include <cstddef> // offsetof

namespace RyoEngine {
	ModelCommon* ModelCommon::GetInstance() {
		static ModelCommon instance;
		return &instance;
	}

	void ModelCommon::Initialize() {
		Logger::Log("ModelCommon : Initializing...\n");
		dxCommon_ = DirectXCommon::GetInstance();
		CreateRootSignature();
		CreateRealPipelineStates();
		CreateReflectPipelineStates();
		CreateNoUVPipelineStates();
		CreateReflectNoUVPipelineStates();
		CreateShadowPipelineState();
		Logger::LogSuccess("ModelCommon : Initialized\n");
	}

	// ブレンドモードに応じた D3D12_BLEND_DESC を生成するヘルパー
	D3D12_BLEND_DESC ModelCommon::CreateBlendDesc(BlendMode blendMode) {
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

		switch (blendMode) {
		case BlendMode::None:
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::Normal: // αブレンド
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::Add: // 加算
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::Subtract: // 減算
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			break;

		case BlendMode::Multiply: // 乗算
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;

		case BlendMode::Screen: // スクリーン
			blendDesc.RenderTarget[0].BlendEnable = TRUE;
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			break;
		}

		return blendDesc;
	}

	void ModelCommon::BeginDraw(DrawType drawType) {
		currentDrawType_ = drawType;

		auto commandList = dxCommon_->GetCommandList();
		commandList->SetGraphicsRootSignature(rootSignature_.Get());

		size_t blendIdx = static_cast<size_t>(blendMode_);

		switch (drawType) {
		case DrawType::REAL:
			commandList->SetPipelineState(realPipelineStates_[blendIdx].Get());
			break;

		case DrawType::REFLECT:
			commandList->SetPipelineState(reflectPipelineStates_[blendIdx].Get());
			break;

		case DrawType::NO_UV:
			commandList->SetPipelineState(noUVPipelineStates_[blendIdx].Get());
			break;

		case DrawType::REFLECT_NO_UV:
			commandList->SetPipelineState(reflectNoUVPipelineStates_[blendIdx].Get());
			break;

		case DrawType::SHADOW:
			commandList->SetPipelineState(shadowPipelineState_.Get());
			break;
		}

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D12DescriptorHeap* ppHeaps[] = { TextureManager::GetInstance()->GetDescriptorHeap() };
		commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		if (drawType == DrawType::SHADOW) {
			commandList->SetGraphicsRootConstantBufferView(6, ShadowMap::GetInstance()->GetLightViewProjGPUVirtualAddress());
		} else {
			commandList->SetGraphicsRootConstantBufferView(6, ShadowMap::GetInstance()->GetLightViewProjGPUVirtualAddress());
			commandList->SetGraphicsRootDescriptorTable(7, TextureManager::GetInstance()->GetGPUHandle(ShadowMap::GetInstance()->GetShadowMapTextureHandle()));
		}
	}

	void ModelCommon::Draw() {
		BeginDraw(DrawType::REAL);
		for (const auto& command : drawCommands_) {
			command();
		}
	}

	void ModelCommon::DrawShadow() {
		BeginDraw(DrawType::SHADOW);
		for (const auto& command : drawCommands_) {
			command();
		}
	}

	void ModelCommon::Finalize() {
		Logger::Log("ModelCommon : Finalizing...\n");
		// グラフィックスパイプラインを解放
		shadowPipelineState_.Reset();
		for (auto& pso : reflectNoUVPipelineStates_) pso.Reset();
		for (auto& pso : noUVPipelineStates_) pso.Reset();
		for (auto& pso : reflectPipelineStates_) pso.Reset();
		for (auto& pso : realPipelineStates_) pso.Reset();

		// ルートシグネチャを解放
		rootSignature_.Reset();

		// 保持していた DirectXCommon のポインタをクリア
		dxCommon_ = nullptr;
		Logger::LogSuccess("ModelCommon : Finaled\n");
	}

	void ModelCommon::CreateRootSignature() {
		HRESULT hr = S_OK;

		// RootSignature作成
		// NOTE: UV無し用PSO(NO_UV / REFLECT_NO_UV)も含め、全PSOでこの同一RootSignatureを使い回す。
		//       UV無し用のPixelShaderはTexture(t0)/Sampler(s0)を参照しないだけで、
		//       RootSignatureにスロットが余分に存在すること自体はPSO生成のエラーにはならない。
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// DescriptorRange
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;  // 0から始まる
		descriptorRange[0].NumDescriptors = 1;  // 数は1つ
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  // SRVを使う
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;  // Offsetを自動計算

		// RootParameterを作成
		D3D12_ROOT_PARAMETER rootParameters[8] = {};
		// Material
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;  // CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;  // レジスタ番号0とバインド
		// WVP
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 0;
		// Texture
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;  // Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);  // Tableで利用する数

		// Light配列 (StructuredBuffer。Root DescriptorのSRVとして直接バインドする。DescriptorHeap登録は不要)
		// NOTE: Textureがt0を使っているため、こちらはt1にする
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;  // t1

		// 有効ライト数 (cbuffer)
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4].Descriptor.ShaderRegister = 1;  // b1

		// アンビエントライト (cbuffer)
		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[5].Descriptor.ShaderRegister = 2;  // b2

		// ライトのView-Projection行列 (シャドウマップ用)
		// NOTE: シャドウパスの頂点シェーダー(World*LightVPの計算)と、通常パスのピクセルシェーダー
		//       (ワールド座標→ライトのクリップ空間への変換)の両方から読むため、VISIBILITY_ALLにする。
		//       Pixel側では既にb1(LightCount)/b2(Ambient)を使っているため、衝突しないb3を使う。
		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParameters[6].Descriptor.ShaderRegister = 3;  // b3 (Vertex/Pixel共通)

		// シャドウマップ本体 (通常パスのピクセルシェーダーがサンプリングする用)
		D3D12_DESCRIPTOR_RANGE shadowDescriptorRange[1] = {};
		shadowDescriptorRange[0].BaseShaderRegister = 2;  // t2 (Textureのt0、Light配列のt1とは別)
		shadowDescriptorRange[0].NumDescriptors = 1;
		shadowDescriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		shadowDescriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[7].DescriptorTable.pDescriptorRanges = shadowDescriptorRange;
		rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(shadowDescriptorRange);

		descriptionRootSignature.pParameters = rootParameters;  // ルートパラメータ配列へのポインタ
		descriptionRootSignature.NumParameters = _countof(rootParameters);  // 配列の長さ

		// Samplerの設定(一般的な設定)
		D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;  // バイリニアフィルタ
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;  // 繰り返す
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;  // 比較しない
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;  // ありったけのMipmapを使う
		staticSamplers[0].ShaderRegister = 0;  // レジスタ番号0を使う
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // PixelShaderで使う

		// シャドウマップ用の比較サンプラー (SampleCmpによるハードウェアPCF用)
		// NOTE: 比較サンプラーはFilterをD3D12_FILTER_COMPARISON_*系にする必要がある。
		//       ComparisonFunc=LESS_EQUALにより、「このピクセルの深度 <= シャドウマップに書かれた深度」
		//       の場合に明るい(影ではない)と判定される。
		staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;  // 範囲外は最大深度(=影なし)扱い
		staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[1].ShaderRegister = 1;  // s1
		staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		// シリアライズしてバイナリにする
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature,
			D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
		if (FAILED(hr)) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			assert(false);
		}
		// バイナリをもとに生成
		hr = dxCommon_->GetDevice()->CreateRootSignature(0,
			signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));

	}

	void ModelCommon::CreateRealPipelineStates() {
		HRESULT hr = S_OK;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model.PS.hlsl", L"ps_6_0");
		assert(vertexShaderBlob != nullptr);
		assert(pixelShaderBlob != nullptr);

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		// 6つのブレンドモード分をループして生成
		for (int i = 0; i < 6; ++i) {
			BlendMode mode = static_cast<BlendMode>(i);
			D3D12_BLEND_DESC blendDesc = CreateBlendDesc(mode);

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
			desc.pRootSignature = rootSignature_.Get();
			desc.InputLayout = inputLayoutDesc;
			desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
			desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
			desc.BlendState = blendDesc;
			desc.RasterizerState = rasterizerDesc;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.SampleDesc.Count = 1;
			desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
			desc.DepthStencilState = depthStencilDesc;
			desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&realPipelineStates_[i]));
			assert(SUCCEEDED(hr));
		}
	}

	void ModelCommon::CreateReflectPipelineStates() {
		// 基本はRealと同様だが RasterizerState.CullMode = D3D12_CULL_MODE_FRONT にする
		HRESULT hr = S_OK;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT; // 反射用
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model.PS.hlsl", L"ps_6_0");

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		for (int i = 0; i < 6; ++i) {
			BlendMode mode = static_cast<BlendMode>(i);
			D3D12_BLEND_DESC blendDesc = CreateBlendDesc(mode);

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
			desc.pRootSignature = rootSignature_.Get();
			desc.InputLayout = inputLayoutDesc;
			desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
			desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
			desc.BlendState = blendDesc;
			desc.RasterizerState = rasterizerDesc;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.SampleDesc.Count = 1;
			desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
			desc.DepthStencilState = depthStencilDesc;
			desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&reflectPipelineStates_[i]));
			assert(SUCCEEDED(hr));
		}
	}

	void ModelCommon::CreateNoUVPipelineStates() {
		HRESULT hr = S_OK;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));
		inputElementDescs[1].SemanticName = "NORMAL";
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, normal));

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model_NoUV.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model_NoUV.PS.hlsl", L"ps_6_0");

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		for (int i = 0; i < 6; ++i) {
			BlendMode mode = static_cast<BlendMode>(i);
			D3D12_BLEND_DESC blendDesc = CreateBlendDesc(mode);

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
			desc.pRootSignature = rootSignature_.Get();
			desc.InputLayout = inputLayoutDesc;
			desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
			desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
			desc.BlendState = blendDesc;
			desc.RasterizerState = rasterizerDesc;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.SampleDesc.Count = 1;
			desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
			desc.DepthStencilState = depthStencilDesc;
			desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&noUVPipelineStates_[i]));
			assert(SUCCEEDED(hr));
		}
	}

	void ModelCommon::CreateReflectNoUVPipelineStates() {
		HRESULT hr = S_OK;

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));
		inputElementDescs[1].SemanticName = "NORMAL";
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, normal));

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model_NoUV.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Model/Model_NoUV.PS.hlsl", L"ps_6_0");

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		for (int i = 0; i < 6; ++i) {
			BlendMode mode = static_cast<BlendMode>(i);
			D3D12_BLEND_DESC blendDesc = CreateBlendDesc(mode);

			D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
			desc.pRootSignature = rootSignature_.Get();
			desc.InputLayout = inputLayoutDesc;
			desc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
			desc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
			desc.BlendState = blendDesc;
			desc.RasterizerState = rasterizerDesc;
			desc.NumRenderTargets = 1;
			desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
			desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			desc.SampleDesc.Count = 1;
			desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
			desc.DepthStencilState = depthStencilDesc;
			desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

			hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&reflectNoUVPipelineStates_[i]));
			assert(SUCCEEDED(hr));
		}
	}

	void ModelCommon::CreateShadowPipelineState() {
		HRESULT hr = S_OK;

		// InputLayout: POSITIONのみ (深度だけ書ければいいのでNORMAL/TEXCOORDは不要)
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = static_cast<UINT>(offsetof(VertexData, position));

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		// RasterizerState
		D3D12_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		// NOTE: シャドウアクネ(自己遮蔽による縞模様)対策の深度バイアス。
		//       D32_FLOATは整数フォーマットと違いDepthBiasの効き方が独特なので、
		//       ここは仮の値。実機で縞模様/影の浮きが出たら調整すること。
		rasterizerDesc.DepthBias = 50;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.SlopeScaledDepthBias = 1.5f;

		// シャドウ専用の頂点シェーダーのみ使用 (ピクセルシェーダーは無し＝深度だけ書く)
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = ShaderCompiler::GetInstance()->Compile(L"Resources/EngineResources/HLSL/Shadow/ShadowMap.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
		graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
		graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
		graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
			vertexShaderBlob->GetBufferSize() };
		// PSは設定しない(深度専用パス)
		graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
		graphicsPipelineStateDesc.NumRenderTargets = 0;  // カラーバッファは使わない
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicsPipelineStateDesc.SampleDesc.Count = 1;
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;  // ShadowMapのDSVフォーマットと一致させる
		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
			IID_PPV_ARGS(&shadowPipelineState_));
		assert(SUCCEEDED(hr));
	}

}