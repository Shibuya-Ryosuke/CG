#include "DirectXCommon.h"
#include "WinApp.h"
#include "Logger.h"
#include "ShaderCompiler.h"
#include <format>
#include <cassert>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Engine{
	DirectXCommon* DirectXCommon::GetInstance() {
		static DirectXCommon instance;
		return &instance;
	}

	void DirectXCommon::Initialize(WinApp* winApp, int32_t width, int32_t height) {
		Logger::Log("DirectXCommon: Initializing");

		backBufferWidth_ = width;
		backBufferHeight_ = height;

		HRESULT hr = S_OK;


		// DXGIファクトリーの生成
		dxgiFactory_ = nullptr;
		hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
		// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
		assert(SUCCEEDED(hr));

		// 使用するアダプタ用の変数。最初にnullptrを入れておく
		Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
		// 良い順にアダプタを組み込む
		for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
			DXGI_ERROR_NOT_FOUND; ++i) {
			// アダプターの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			hr = useAdapter->GetDesc3(&adapterDesc);
			assert(SUCCEEDED(hr)); // 取得できないのは一大事
			// ソフトウェアアダプタでなければ採用！
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
				// 採用したアダプタの情報をログに出力。wstringのほうなので注意
				Logger::Log("DirectXCommon: Using Adapter -> " + Logger::ConvertString(adapterDesc.Description));
				break;
			}
			useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
		}
		// 適切なアダプタが見つからなかったので起動できない
		assert(useAdapter != nullptr);



		// D3D12Deviceの生成
		device_ = nullptr;
		// 機能レベルとログ出力用の文字列
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
		};
		const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
		// 高い順に生成できるか試していく
		for (size_t i = 0; i < _countof(featureLevels); ++i) {
			// 採用したアダプターでデバイス生成
			hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
			// 指定した機能レベルでデバイスが生成できたか確認
			if (SUCCEEDED(hr)) {
				// 生成できたのでログ出力を行ってループを抜ける
				Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
				break;
			}
		}
		// デバイスの生成が上手くいかなかったので起動できない
		assert(device_ != nullptr);
		// 初期化完了のログを出力
		Logger::Log("DirectXCommon: Device created.\n");

        #ifdef _DEBUG
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
		if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
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
		hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
		// コマンドキューの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));

		// コマンドアロケータ生成
		hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
		// コマンドアロケータの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));

		// コマンドリストを生成する
		hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
		// コマンドリストの生成が上手くいかなかったので起動できない
		assert(SUCCEEDED(hr));



		// スワップチェーンを生成する
		swapChain_ = nullptr;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = static_cast<UINT>(backBufferWidth_);    // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Height = static_cast<UINT>(backBufferHeight_);  // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
		swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
		swapChainDesc.BufferCount = 2;  // ダブルバッファ
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // モニタに移したら、中身を破棄
		// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
		hr = dxgiFactory_->CreateSwapChainForHwnd(commandQueue_.Get(), winApp->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
		// スワップチェーンが生成できなかったので起動できない
		assert(SUCCEEDED(hr));


		// ディスクリプタヒープ生成
		// DescriptorSizeを取得
		const uint32_t descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// ディスクリプタヒープの作成(RTV用)
		rtvHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);



		// SwapChainからResourceを引っ張ってくる
		hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
		// 上手く取得できなければ起動できない
		assert(SUCCEEDED(hr));
		hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
		assert(SUCCEEDED(hr));

		// RTVの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  // 出力結果をSRGBに変換して書き込む
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;  // 2dテクスチャとして書き込む
		// ディスクリプタの先頭を取得する
		D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = GetCPUDescriptorHandle(rtvHeap_, descriptorSizeRTV, 0);
		// RTVを2つ作るのでディスクリプタを2つ用意
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
		// ます1つ目を作る。1つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
		rtvHandles[0] = rtvStartHandle;
		for (uint32_t i = 0; i < 2; ++i) {
			rtvHandles[i] = GetCPUDescriptorHandle(rtvHeap_, descriptorSizeRTV, i);
			device_->CreateRenderTargetView(swapChainResources_[i].Get(), &rtvDesc, rtvHandles[i]);
		}



		// 初期値0でFenceを作る
		fence_ = nullptr;
		fenceValue_ = 0;
		hr = device_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
		assert(SUCCEEDED(hr));

		// Fenceのsignalを待つためのイベントを作成する
		fenceEvent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
		assert(fenceEvent_ != nullptr);
	}

	void DirectXCommon::Finalize() {
		// 処理待ち
		// Fenceの値を更新
		fenceValue_++;
		// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにsignalを送る
		commandQueue_->Signal(fence_.Get(), fenceValue_);
		// Fenceの値が指定したSignal値にたどり着いているか確認する
		// GetCompletedValueの初期値はFence作成時に渡した初期値
		if (fence_->GetCompletedValue() < fenceValue_) {
			// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
			fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
			// イベントを待つ
			WaitForSingleObject(fenceEvent_, INFINITE);
		}

		// 解放
		if (fenceEvent_ != nullptr) {
			CloseHandle(fenceEvent_);
			fenceEvent_ = nullptr;
		}

		// シェーダーコンパイラ
	

		// フェンス
		fence_.Reset();

		// 深度バッファ
		dsvHeap_.Reset();
		depthStencilResource_.Reset();

		// レンダーターゲット / スワップチェーン
		rtvHeap_.Reset();
		for (int i = 0; i < 2; ++i) {
			swapChainResources_[i].Reset();
		}
		swapChain_.Reset();

		// コマンド周り
		commandList_.Reset();
		commandAllocator_.Reset();
		commandQueue_.Reset();

		// デバイス周り（これが最後に消える必要がある）
		dxgiFactory_.Reset();
		device_.Reset();
	}

	void DirectXCommon::PreRender() {
		// これから書き込むバックバッファのインデックスを取得
		uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

		// リソースバリアの設定(表示用から描画用へ切り替え)
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;      // 表示状態から
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // 描画状態へ
		commandList_->ResourceBarrier(1, &barrier);

		// 描画先の設定(RTVとDSV)
		// RTVのハンドルを取得
		const uint32_t descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvHeap_, descriptorSizeRTV, backBufferIndex);

		// DSVのハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

		// コマンドリストにセット
		commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// 指定した色で画面全体をクリアする
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };  // 青っぽい色。RGBAの順
		commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// ビューポートとシザー矩形の設定
		D3D12_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(backBufferWidth_);
		viewport.Height = static_cast<float>(backBufferHeight_);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissorRect{};
		scissorRect.left = 0;
		scissorRect.right = backBufferWidth_;
		scissorRect.top = 0;
		scissorRect.bottom = backBufferHeight_;

		commandList_->RSSetViewports(1, &viewport);
		commandList_->RSSetScissorRects(1, &scissorRect);
	}


	void DirectXCommon::PostRender() {
		// 現在のバックバッファのインデックスを取得
		uint32_t backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

		// リソースバリア
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList_->ResourceBarrier(1, &barrier);

		// コマンドリストを閉じて実行
		HRESULT hr = commandList_->Close();
		assert(SUCCEEDED(hr));

		// コマンドキューに実行を依頼
		ID3D12CommandList* commandLists[] = { commandList_.Get() };
		commandQueue_->ExecuteCommandLists(1, commandLists);

		// GPUとOSに画面の交換を行うよう通知する
		hr = swapChain_->Present(1, 0);
		assert(SUCCEEDED(hr));

		// Fenceの値を更新
		fenceValue_++;
		// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにsignalを送る
		commandQueue_->Signal(fence_.Get(), fenceValue_);
		// Fenceの値が指定したSignal値にたどり着いているか確認する
		// GetCompletedValueの初期値はFence作成時に渡した初期値
		if (fence_->GetCompletedValue() < fenceValue_) {
			// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
			fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
			// イベントを待つ
			WaitForSingleObject(fenceEvent_, INFINITE);
		}

		// 次のフレーム用のコマンドリストを準備
		hr = commandAllocator_->Reset();
		assert(SUCCEEDED(hr));
		hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
		assert(SUCCEEDED(hr));
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
		const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		handleCPU.ptr += (descriptorSize * index);
		return handleCPU;
	}


}