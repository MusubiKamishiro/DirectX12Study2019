#include "Dx12Wrapper.h"
#include "Application.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

void Dx12Wrapper::InitFeatureLevel(HRESULT& result)
{
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// レベルの高いものから検証し、成功したレベルを適用する
	for (auto& l : levels)
	{
		// ディスプレイアダプターを表すデバイスの作成
		result = D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev));

		if (SUCCEEDED(result))
		{
			break;
		}
	}
}

void Dx12Wrapper::CreateSwapChain(HRESULT& result, HWND hwnd)
{
	// DXGIファクトリの作成
	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	// コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&cmdQueue));

	// スワップチェインの作成
	auto wsize = Application::Instance().GetWindowSize();
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = wsize.width;
	swapChainDesc.Height = wsize.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Stereo = false;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.BufferCount = 2;			// バックバッファの数

	result = dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)(&swapChain));
}

void Dx12Wrapper::CreateRenderTarget(HRESULT& result)
{
	// レンダーターゲット作成のための前準備
	// 表示画面用メモリ確保
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// レンダーターゲットビュー
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = 2;	// 表画面と裏画面
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));


	DXGI_SWAP_CHAIN_DESC swapCDesc = {};
	swapChain->GetDesc(&swapCDesc);
	int renderTargetsNum = swapCDesc.BufferCount;
	// レンダーターゲット数ぶん確保
	renderTargets.resize(renderTargetsNum);
	// レンダーターゲットの作成
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// 各ﾃﾞｽｸﾘﾌﾟﾀｰの使用するｻｲｽﾞを計算しとく
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < renderTargetsNum; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));	// キャンバスを取得
		dev->CreateRenderTargetView(renderTargets[i], nullptr, cpuDescH);	// キャンバスと職人を紐づける
		cpuDescH.ptr += rtvSize;	// レンダーターゲットビューのサイズぶんずらす
	}
}

void Dx12Wrapper::InitScreen()
{
	// 画面のクリア
	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto bbIdx = swapChain->GetCurrentBackBufferIndex();		// ﾊﾞｯｸﾊﾞｯﾌｧｲﾝﾃﾞｯｽｸを調べる
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };				// クリアカラー設定
	cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);		// レンダーターゲット設定
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// クリア
	cmdList->Close();
}

void Dx12Wrapper::ExecuteCmd()
{
	ID3D12CommandList* cmdLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
{
	HRESULT result = S_OK;

	InitFeatureLevel(result);
	CreateSwapChain(result, hwnd);
	CreateRenderTarget(result);
	
	// ｺﾏﾝﾄﾞｱﾛｹｰﾀとｺﾏﾝﾄﾞﾘｽﾄの生成
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// 命令を呼ぶ前にリセット
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator, nullptr);

	InitScreen();
	ExecuteCmd();

	swapChain->Present(0, 0);
}


Dx12Wrapper::~Dx12Wrapper()
{
}

void Dx12Wrapper::Update()
{
}
