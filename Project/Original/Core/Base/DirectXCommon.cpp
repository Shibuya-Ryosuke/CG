#include "DirectXCommon.h"
#include "WinApp.h"
#include "Logger.h"
#include "../../ImGui/ImGuiManager.h"
#include "ShaderCompiler.h"
#include "SrvManager.h"
#include <format>
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace RyoEngine {
	DirectXCommon* DirectXCommon::GetInstance() {
		static DirectXCommon instance;
		return &instance;
	}

	void DirectXCommon::Initialize(int32_t width, int32_t height) {
		DirectXCommon* instance = GetInstance();

		Logger::Log("DxCommon : Initializing...\n");

		instance->backBufferWidth_ = width;
		instance->backBufferHeight_ = height;

		HRESULT hr = S_OK;


		// DXGIファクトリーの生成
		instance->dxgiFactory_ = nullptr;
		hr = CreateDXGIFactory(IID_PPV_ARGS(&instance->dxgiFactory_));
		// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
		assert(SUCCEEDED(hr));

		// 使用するアダプタ用の変数。最初にnullptrを入れておく
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
		// 良い順にアダプタを組み込む
		for (UINT i = 0; instance->dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
			DXGI_ERROR_NOT_FOUND; ++i) {
			// アダプターの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			hr = useAdapter->GetDesc3(&adapterDesc);
			assert(SUCCEEDED(hr)); // 取得できないのは一大事
			// ソフトウェアアダプタでなければ採用！
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
				// 採用したアダプタの情報をログに出力。wstringのほうなので注意
				Logger::Log("DxCommon : Using Adapter -> " + Logger::ConvertString(adapterDesc.Description));
				break;
			}
			useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
		}
		// 適切なアダプタが見つからなかったので起動できない
		assert(useAdapter != nullptr);



		// D3D12Deviceの生成
		instance->device_ = nullptr;
		// 機能レベルとログ出力用の文字列
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
		};
		const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
		// 高い順に生成できるか試していく
		for (size_t i = 0; i < _countof(featureLevels); ++i) {
			// 採用したアダプターでデバイス生成
			hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&instance->device_));
			// 指定した機能レベルでデバイスが生成できたか確認
			if (SUCCEEDED(hr)) {
				// 生成できたのでログ出力を行ってループを抜ける
				Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
				break;
			}
		}
		// デバイスの生成が上手くいかなかったので起動できない
		assert(instance->device_ != nullptr);
		// 初期化完了のログを出力
		Logger::LogSuccess("DxCommon : Device created.\n");

#ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
		if (SUCCEEDED(instance->device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
			// ヤバいエラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			// エラー時に止まる
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			//// 警告時に止まる
			//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			//
			// 抑制するメッセージのID
			D3D12_MESSAGE_ID denyIds[] = {
				// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
				// https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
			};
			// 抑制レベル
			D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = _countof(denyIds);
			filter.DenyList.pIDList = denyIds;
			filter.DenyList.NumSeverities = _countof(severities);
			filter.DenyList.pSeverityList = severities;
			// 指定したメッセージの表示を抑制する
			infoQueue->PushStorageFilter(&filter);
		}
#endif


		// コマンドキュー生成
		D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
		hr = instance->device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&instance->commandQueue_));
		// コマンドキューの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));

		// コマンドアロケータ生成
		hr = instance->device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&instance->commandAllocator_));
		// コマンドアロケータの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));

		// コマンドリストを生成する
		hr = instance->device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, instance->commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&instance->commandList_));
		// コマンドリストの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));



		// スワップチェーンを生成する
		instance->swapChain_ = nullptr;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = static_cast<UINT>(instance->backBufferWidth_);    // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Height = static_cast<UINT>(instance->backBufferHeight_);  // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
		swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
		swapChainDesc.BufferCount = 2;  // ダブルバッファ
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // モニタに移したら、中身を破棄
		// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
		hr = instance->dxgiFactory_->CreateSwapChainForHwnd(instance->commandQueue_.Get(), WinApp::GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(instance->swapChain_.GetAddressOf()));
		// スワップチェーンが生成できなかったので起動できない
		assert(SUCCEEDED(hr));


		// ディスクリプタヒープ生成
		// DescriptorSizeを取得
		const uint32_t descriptorSizeRTV = instance->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// ディスクリプタヒープの作成(RTV用)
		instance->rtvHeap_ = CreateDescriptorHeap(instance->device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);



		// SwapChainからResourceを引っ張ってくる
		hr = instance->swapChain_->GetBuffer(0, IID_PPV_ARGS(&instance->swapChainResources_[0]));
		// 上手く取得できなければ起動できない
		assert(SUCCEEDED(hr));
		hr = instance->swapChain_->GetBuffer(1, IID_PPV_ARGS(&instance->swapChainResources_[1]));
		assert(SUCCEEDED(hr));

		// RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  // 出力結果をSRGBに変換して書き込む
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  // 2dテクスチャとして書き込む
		// ディスクリプタの先頭を取得する
		D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = GetCPUDescriptorHandle(instance->rtvHeap_, descriptorSizeRTV, 0);
		// RTVを2つ作るのでディスクリプタを2つ用意
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
		// ます1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
		rtvHandles[0] = rtvStartHandle;
		for (uint32_t i = 0; i < 2; ++i) {
			rtvHandles[i] = GetCPUDescriptorHandle(instance->rtvHeap_, descriptorSizeRTV, i);
			instance->device_->CreateRenderTargetView(instance->swapChainResources_[i].Get(), &rtvDesc, rtvHandles[i]);
		}

		// 深度バッファの生成
		instance->CreateDepthStencilView();


		// 初期値0でFenceを作る
		instance->fence_ = nullptr;
		instance->fenceValue_ = 0;
		hr = instance->device_->CreateFence(instance->fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&instance->fence_));
		assert(SUCCEEDED(hr));

		// Fenceのsignalを待つためのイベントを作成する
		instance->fenceEvent_ = CreateEvent(nullptr, false, false, nullptr);
		assert(instance->fenceEvent_ != nullptr);

		Logger::LogSuccess("DxCommon : Initialized\n");

	}

	void DirectXCommon::Finalize() {
		DirectXCommon* instance = GetInstance();

		Logger::Log("DxCommon : Finalizing...\n");
		// 処理待ち
		// Fenceの値を更新
		instance->fenceValue_++;
		// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにsignalを送る
		instance->commandQueue_->Signal(instance->fence_.Get(), instance->fenceValue_);
		// Fenceの値が指定したSignal値にたどり着いているか確認する
		// GetCompletedValueの初期値はFence作成時に渡した初期値
		if (instance->fence_->GetCompletedValue() < instance->fenceValue_) {
			// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
			instance->fence_->SetEventOnCompletion(instance->fenceValue_, instance->fenceEvent_);
			// イベントを待つ
			WaitForSingleObject(instance->fenceEvent_, INFINITE);
		}

		// 解放
		if (instance->fenceEvent_ != nullptr) {
			CloseHandle(instance->fenceEvent_);
			instance->fenceEvent_ = nullptr;
		}

		// シェーダーコンパイラ


		instance->gameRtvHeap_.Reset();
		instance->gameRenderTargetResource_.Reset();
		// フェンス
		instance->fence_.Reset();

		// 深度バッファ
		instance->dsvDescriptorHeap_.Reset();
		instance->depthStencilResource_.Reset();

		// レンダーターゲット / スワップチェーン
		instance->rtvHeap_.Reset();
		for (int i = 0; i < 2; ++i) {
			instance->swapChainResources_[i].Reset();
		}
		instance->swapChain_.Reset();

		// コマンド周り
		instance->commandList_.Reset();
		instance->commandAllocator_.Reset();
		instance->commandQueue_.Reset();

		// デバイス周り（これが最後に消える必要がある）
		instance->dxgiFactory_.Reset();
		instance->device_.Reset();
		Logger::LogSuccess("DxCommon Finalized\n");
	}

	void DirectXCommon::PreDraw() {
		DirectXCommon* instance = GetInstance();

#ifdef _DEBUG
		// --- [Debug時] ゲーム用テクスチャへ描画 ---
		D3D12_RESOURCE_BARRIER gameTexBarrier{};
		gameTexBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		gameTexBarrier.Transition.pResource = instance->gameRenderTargetResource_.Get();
		gameTexBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		gameTexBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		instance->commandList_->ResourceBarrier(1, &gameTexBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = instance->gameRtvHeap_->GetCPUDescriptorHandleForHeapStart();
#else
		// --- [Release時] 直接スワップチェーンへ描画 ---
		uint32_t backBufferIndex = instance->swapChain_->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER swapChainBarrier{};
		swapChainBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		swapChainBarrier.Transition.pResource = instance->swapChainResources_[backBufferIndex].Get();
		swapChainBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		swapChainBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		instance->commandList_->ResourceBarrier(1, &swapChainBarrier);

		// 既存のRTVディスクリプタヒープからハンドルを計算
		const uint32_t descriptorSizeRTV = instance->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(instance->rtvHeap_, descriptorSizeRTV, backBufferIndex);
#endif
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = instance->dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

		instance->commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// クリア処理
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
		instance->commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		instance->commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// ビューポートとシザー矩形の設定
		D3D12_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(instance->backBufferWidth_);
		viewport.Height = static_cast<float>(instance->backBufferHeight_);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect{};
		scissorRect.left = 0;
		scissorRect.right = instance->backBufferWidth_;
		scissorRect.top = 0;
		scissorRect.bottom = instance->backBufferHeight_;

		instance->commandList_->RSSetViewports(1, &viewport);
		instance->commandList_->RSSetScissorRects(1, &scissorRect);
	}


	void DirectXCommon::PostDraw() {
		DirectXCommon* instance = GetInstance();

		uint32_t backBufferIndex = instance->swapChain_->GetCurrentBackBufferIndex();

#ifdef _DEBUG
		// --- [Debug時] ゲーム用テクスチャからスワップチェーンへ切り替えてImGuiを描画 ---
		D3D12_RESOURCE_BARRIER gameTexBarrier{};
		gameTexBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		gameTexBarrier.Transition.pResource = instance->gameRenderTargetResource_.Get();
		gameTexBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		gameTexBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		instance->commandList_->ResourceBarrier(1, &gameTexBarrier);

		D3D12_RESOURCE_BARRIER swapChainBarrier{};
		swapChainBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		swapChainBarrier.Transition.pResource = instance->swapChainResources_[backBufferIndex].Get();
		swapChainBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		swapChainBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		instance->commandList_->ResourceBarrier(1, &swapChainBarrier);

		const uint32_t descriptorSizeRTV = instance->device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE mainRtvHandle = GetCPUDescriptorHandle(instance->rtvHeap_, descriptorSizeRTV, backBufferIndex);
		instance->commandList_->OMSetRenderTargets(1, &mainRtvHandle, false, nullptr);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		instance->commandList_->ClearRenderTargetView(mainRtvHandle, clearColor, 0, nullptr);

		// ImGuiの描画コマンド発行 (Release時は空関数になるので呼ばれても安全ですが、ifdefで囲むとより明確です)
		ImGuiManager::EndFrame(instance->commandList_.Get());
#endif
		// =================================================================
		// ⑥ [既存の処理] スワップチェーンを「描画ターゲット」から「表示状態(PRESENT)」に戻す
		// =================================================================
		D3D12_RESOURCE_BARRIER presentBarrier{};
		presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentBarrier.Transition.pResource = instance->swapChainResources_[backBufferIndex].Get();
		presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		instance->commandList_->ResourceBarrier(1, &presentBarrier);

		// =================================================================
		// ⑦ [既存の処理] コマンドリストを閉じて実行 (ここから下は元のコードのまま)
		// =================================================================
		HRESULT hr = instance->commandList_->Close();
		assert(SUCCEEDED(hr));

		ID3D12CommandList* commandLists[] = { instance->commandList_.Get() };
		instance->commandQueue_->ExecuteCommandLists(1, commandLists);

		hr = instance->swapChain_->Present(1, 0);
		assert(SUCCEEDED(hr));

		// フェンス同期 (残す！)
		instance->fenceValue_++;
		instance->commandQueue_->Signal(instance->fence_.Get(), instance->fenceValue_);
		if (instance->fence_->GetCompletedValue() < instance->fenceValue_) {
			instance->fence_->SetEventOnCompletion(instance->fenceValue_, instance->fenceEvent_);
			WaitForSingleObject(instance->fenceEvent_, INFINITE);
		}

		// コマンドリストとアロケータのリセット (残す！)
		hr = instance->commandAllocator_->Reset();
		assert(SUCCEEDED(hr));
		hr = instance->commandList_->Reset(instance->commandAllocator_.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}

	void DirectXCommon::CreateGameRenderTarget() {
		DirectXCommon* instance = GetInstance();

		Logger::Log("* Creating GameRenderTarget... *\n");
		HRESULT hr = S_OK;

		// 1. レンダーターゲットとして使えるテクスチャリソースの設定
		D3D12_RESOURCE_DESC texDesc{};
		texDesc.Width = instance->backBufferWidth_;
		texDesc.Height = instance->backBufferHeight_;
		texDesc.MipLevels = 1;
		texDesc.DepthOrArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // ImGuiで扱いやすいフォーマット
		texDesc.SampleDesc.Count = 1;
		texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		// ★重要: レンダーターゲットとして使用可能にするフラグ
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_PROPERTIES heapProps{};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上

		// クリア最適値の設定
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = texDesc.Format;
		clearValue.Color[0] = 0.1f; // PreDrawのクリア色と合わせる
		clearValue.Color[1] = 0.25f;
		clearValue.Color[2] = 0.5f;
		clearValue.Color[3] = 1.0f;

		// リソース生成 (最初は描画ターゲット状態にしておく)
		hr = instance->device_->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&instance->gameRenderTargetResource_)
		);
		assert(SUCCEEDED(hr));

		// 2. 専用のRTVディスクリプタヒープを作成
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.NumDescriptors = 1;
		hr = instance->device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&instance->gameRtvHeap_));
		assert(SUCCEEDED(hr));

		// RTVの作成
		instance->device_->CreateRenderTargetView(instance->gameRenderTargetResource_.Get(), nullptr, instance->gameRtvHeap_->GetCPUDescriptorHandleForHeapStart());

		// 3. SrvManagerでスロットを確保し、ImGui::Image等で表示するためのSRVを作る
		//    (SrvManager::Initialize()より後に呼ぶこと)
		instance->gameTextureSrvIndex_ = SrvManager::Allocate();
		SrvManager::CreateSRVForTexture2D(instance->gameTextureSrvIndex_, instance->gameRenderTargetResource_.Get(), texDesc.Format, 1);

		Logger::LogSuccess("* Created *\n");
	}

	void DirectXCommon::WaitForFence() {
		DirectXCommon* instance = GetInstance();

		instance->fenceValue_++;
		instance->commandQueue_->Signal(instance->fence_.Get(), instance->fenceValue_);
		if (instance->fence_->GetCompletedValue() < instance->fenceValue_) {
			instance->fence_->SetEventOnCompletion(instance->fenceValue_, instance->fenceEvent_);
			WaitForSingleObject(instance->fenceEvent_, INFINITE);
		}
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
		ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType;
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
		// ディスクリプタヒープが作れなかったので起動できない
		assert(SUCCEEDED(hr));
		return descriptorHeap;
	}

	// CPUのDescriptorHandleを取得
	D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index) {
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes) {
		// 【重要】CBVは256バイトアライメントが必要なので、サイズを調整する
		// (n + 255) & ~255 という計算で、256の倍数に切り上げられます
		size_t alignmentSize = (sizeInBytes + 0xFF) & ~0xFF;

		// リソース用のヒープの設定
		D3D12_HEAP_PROPERTIES uploadHeapProperties{};
		uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う
		// リソースの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		// バッファリソース。テクスチャの場合はまた別の設定をする
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = alignmentSize;
		// バッファの場合はこれらは1にする決まり
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		// バッファの場合はこれにする決まり
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		// 実際に頂点リソースを作る
		Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
		HRESULT hr = device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));

		return resource;
	}

	void DirectXCommon::CreateDepthStencilView() {
		// 生成するResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = backBufferWidth_;  // Textureの幅
		resourceDesc.Height = backBufferHeight_;  // Textureの高さ
		resourceDesc.MipLevels = 1;  // mipmapの数
		resourceDesc.DepthOrArraySize = 1;  // 奥行き or 配列Textureの数
		resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // DepthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1;  // サンプリングのカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // Textureの次元数。2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;  // DepthStencilとして使う通知

		// 利用するHeapの設定
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;  // VRAM上に作る

		// 深度値のクリア設定
		D3D12_CLEAR_VALUE depthClearValue{};
		depthClearValue.DepthStencil.Depth = 1.0f;  // 1.0f(最大値)でクリア
		depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // フォーマット。Resourceと合わせる

		// Resourceの生成
		HRESULT hr = device_->CreateCommittedResource(
			&heapProperties,  // Heapの設定
			D3D12_HEAP_FLAG_NONE,  // Heapの特殊な設定。特になし
			&resourceDesc,  // Resourceの設定
			D3D12_RESOURCE_STATE_DEPTH_WRITE,  // 深度阿多を書き込む状態にしておく
			&depthClearValue,  // Clear最適値
			IID_PPV_ARGS(&depthStencilResource_)  // 作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));

		// DSV用ディスクリプタヒープの作成
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.NumDescriptors = 1;
		hr = device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap_));
		assert(SUCCEEDED(hr));

		// DSVの設定
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;  // Format。基本的にはResourceに合わせる
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;  // 2dTexture
		// DSVHeapの先頭にDSVを作る
		device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
	}


}