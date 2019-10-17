#include "Dx12Wrapper.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "d3dx12.h"

#include "Application.h"
#include "Dx12Device.h"
#include "Dx12Constants.h"
#include "PMDManager.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")


void Dx12Wrapper::CreateDebugLayer()
{
	ID3D12Debug* debug;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (SUCCEEDED(result))
	{
		debug->EnableDebugLayer();
	}
	debug->Release();
}

void Dx12Wrapper::CreateSwapChain(HWND hwnd)
{
	// コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	auto result = Dx12Device::Instance().GetDevice()->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&cmdQueue));

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

	result = Dx12Device::Instance().GetDxgiFactory()->CreateSwapChainForHwnd(
		cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)(&swapChain));
}

void Dx12Wrapper::CreateRenderTarget()
{
	auto dev = Dx12Device::Instance().GetDevice();

	// レンダーターゲット作成のための前準備
	// 表示画面用メモリ確保
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// レンダーターゲットビュー
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = 2;	// 表画面と裏画面
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));


	DXGI_SWAP_CHAIN_DESC swapCDesc = {};
	swapChain->GetDesc(&swapCDesc);
	// レンダーターゲット数ぶん確保
	backBuffers.resize(swapCDesc.BufferCount);
	// レンダーターゲットの作成
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// 各ﾃﾞｽｸﾘﾌﾟﾀｰの使用するｻｲｽﾞを計算しとく
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < static_cast<int>(swapCDesc.BufferCount); ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));	// キャンバスを取得
		dev->CreateRenderTargetView(backBuffers[i], nullptr, cpuDescH);		// キャンバスと職人を紐づける
		cpuDescH.ptr += rtvSize;	// レンダーターゲットビューのサイズぶんずらす
	}
}

void Dx12Wrapper::CreateVertexBuffer()
{
	std::vector<Vertex> vertices;
	vertices.resize(4);
	vertices.emplace_back(DirectX::XMFLOAT3(-0.8f, -0.8f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));	// 左下
	vertices.emplace_back(DirectX::XMFLOAT3(-0.8f, 0.8f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f));		// 左上
	vertices.emplace_back(DirectX::XMFLOAT3(0.8f, -0.8f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));		// 右上
	vertices.emplace_back(DirectX::XMFLOAT3(0.8f, 0.8f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));		// 右下

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = vertices.size();	// 頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	
	auto dev = Dx12Device::Instance().GetDevice();
	// 頂点バッファの作成
	auto result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc,
								D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	std::vector<unsigned short> indices = { 0,1,2, 1,2,3 };	// 頂点を使用する順番
	// インデックスバッファの作成
	result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	D3D12_RANGE range = { 0,0 };
	Vertex* vertexMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)& vertexMap);
	std::copy(std::begin(vertices), std::end(vertices), vertexMap);
	vertexBuffer->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(Vertex);	// 頂点1つあたりのバイト数
	vbView.SizeInBytes = sizeof(vertices);	// データ全体のサイズ

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	// バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	// フォーマット(shortなのでR16)
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);		// 総サイズ
}

void Dx12Wrapper::InitShader()
{
	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);

	InitRootSignatur();
	InitPipelineState();

	auto wsize = Application::Instance().GetWindowSize();
	// ビューポート設定
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(wsize.width);
	viewport.Height = static_cast<FLOAT>(wsize.height);
	viewport.MaxDepth = 1.0f;	// カメラからの距離(遠いほう)
	viewport.MinDepth = 0.0f;	// カメラからの距離(近いほう)

	// シザー(切り取り)矩形設定
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = wsize.width;
	scissorRect.bottom = wsize.height;
}

void Dx12Wrapper::InitRootSignatur()
{
	D3D12_DESCRIPTOR_RANGE descRange[3] = {};
	// "b0"	wvp
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[0].BaseShaderRegister = 0;	// レジスタ番号
	descRange[0].NumDescriptors = 1;		// 1回で読む数
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "b1"	マテリアル
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	// 定数バッファ
	descRange[1].BaseShaderRegister = 1;	// レジスタ番号
	descRange[1].NumDescriptors = 1;		// 1回で読む数
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "t0" マテリアルにはるテクスチャ
	// 通常テクスチャ, 加算テクスチャ, 乗算テクスチャ, トゥーンテクスチャを一括で読む
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[2].BaseShaderRegister = 0;	// レジスタ番号
	descRange[2].NumDescriptors = 4;		// 1回で読む数
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	

	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;		// レンジの数
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];	// 対応するレンジへのポインタ
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;		// レンジの数
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];	// 対応するレンジへのポインタ
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	// 特別なフィルタを使用しない
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// 絵が繰り返される(U方向)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// 絵が繰り返される(V方向)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// 絵が繰り返される(W方向)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;		// MIPMAP上限なし
	samplerDesc.MinLOD = 0.0f;					// MIPMAP下限なし
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// エッジのいろ(黒透明)
	samplerDesc.ShaderRegister = 0;				// 使用するシェーダレジスタ(スロット)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 全部のデータをシェーダに見せる
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 2;
	rsd.NumStaticSamplers = 1;
	rsd.pParameters = &rootParam[0];
	rsd.pStaticSamplers = &samplerDesc;

	ID3DBlob* signature = nullptr;		// ルートシグネチャをつくるための材料
	ID3DBlob* error = nullptr;			// エラー出た時の対処
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ルートシグネチャの作成
	result = Dx12Device::Instance().GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void Dx12Wrapper::InitPipelineState()
{
	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC layouts[] = { 
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, 
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// パイプラインステートを作る
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	// ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layouts;
	gpsDesc.InputLayout.NumElements = _countof(layouts);

	// シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);		// 頂点シェーダ
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);		// ピクセルシェーダ

	// レンダターゲット
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;		// このターゲット数と設定するフォーマット数は
	gpsDesc.NumRenderTargets = 1;							// 一致させておく

	// 深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;		// あとで
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;				// 必須(DSV)
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// DSV必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さいほうを返す

	// ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 表面だけじゃなくて、裏面も描画するようにするよ

	// その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;		// いる
	gpsDesc.SampleDesc.Quality = 0;		// いる
	gpsDesc.SampleMask = 0xffffffff;	// 全部1
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形

	auto result = Dx12Device::Instance().GetDevice()->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
}

void Dx12Wrapper::CreateDepthBuff()
{
	auto wsize = Application::Instance().GetWindowSize();

	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.width;
	depthResDesc.Height = wsize.height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthResDesc.MipLevels = 1;

	// このクリアバリューが重要な意味を持つので今回は作っておく
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;	// 深さ最大値は1
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	auto dev = Dx12Device::Instance().GetDevice();
	// 深度バッファの作成
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&depthBuff));
	

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};	// 特に設定の必要はない
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
	_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

	// 深度バッファビューの作成
	dev->CreateDepthStencilView(depthBuff, &_dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::ClearCmd(ID3D12PipelineState* pipelinestate, ID3D12RootSignature* rootsignature)
{
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator, pipelinestate);
	cmdList->SetGraphicsRootSignature(rootsignature);
}

void Dx12Wrapper::UnlockBarrier(ID3D12Resource* buffer)
{
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = buffer;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// バリアを解除
	cmdList->ResourceBarrier(1, &BarrierDesc);
}

void Dx12Wrapper::SetBarrier()
{
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// バリアをセット
	cmdList->ResourceBarrier(1, &BarrierDesc);
}

void Dx12Wrapper::ExecuteCmd()
{
	ID3D12CommandList* cmdLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);
	cmdQueue->Signal(fence, ++fenceValue);
}

void Dx12Wrapper::WaitExecute()
{
	while (fence->GetCompletedValue() < fenceValue)
	{
		;// 待つだけなので何もしないよ
	}
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
{
#ifdef _DEBUG
	CreateDebugLayer();
#endif // _DEBUG

	auto dev = Dx12Device::Instance().GetDevice();
	CreateSwapChain(hwnd);
	CreateRenderTarget();
	
	// コマンドアロケータとコマンドリストの生成
	auto result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
	// フェンスの作成
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	
	//modelPath = "model/vocaloid/初音ミク.pmd";
	//modelPath = "model/vocaloid/初音ミクmetal.pmd";
	modelPath = "model/vocaloid/巡音ルカ.pmd";
	//modelPath = "model/yayoi/やよいヘッド_カジュアル（体x0.96）改造.pmd";
	//modelPath = "model/hibiki/我那覇響v1.pmd";
	pmdManager.reset(new PMDManager(modelPath));

	CreateDepthBuff();
	
	CreateVertexBuffer();

	InitShader();


	cmdList->Close();
}

Dx12Wrapper::~Dx12Wrapper()
{
	cmdAllocator->Release();
	cmdList->Release();
	cmdQueue->Release();
	swapChain->Release();
	fence->Release();
	vertexShader->Release();
	pixelShader->Release();
	rootSignature->Release();
	pipelineState->Release();

	rtvDescriptorHeap->Release();
	vertexBuffer->Release();
	indexBuffer->Release();
	for (auto& backBuffer : backBuffers)
	{
		backBuffer->Release();
	}

	depthBuff->Release();
	dsvHeap->Release();
}

void Dx12Wrapper::Update()
{
	// 入力関連
	unsigned char keyState[256] = {};

	Vector3 pos = {};
	Vector3 angle = {};

	if (GetKeyboardState(keyState))
	{
		// 平行移動
		if (keyState[VK_UP] & 0x80)	// 0b00000000
		{
			pos.y = 0.05f;
		}
		if (keyState[VK_DOWN] & 0x80)
		{
			pos.y = -0.05f;
		}
		if (keyState[VK_RIGHT] & 0x80)
		{
			pos.x = 0.05f;
		}
		if (keyState[VK_LEFT] & 0x80)
		{
			pos.x = -0.05f;
		}
		if (keyState['Z'] & 0x80)
		{
			pos.z = 0.05f;
		}
		if (keyState['X'] & 0x80)
		{
			pos.z = -0.05f;
		}

		// 回転
		if (keyState['A'] & 0x80)
		{
			angle.x = 0.01f;
		}
		if (keyState['D'] & 0x80)
		{
			angle.x = -0.01f;
		}
		if (keyState['W'] & 0x80)
		{
			angle.y = 0.01f;
		}
		if (keyState['S'] & 0x80)
		{
			angle.y = -0.01f;
		}
	}

	auto m = Dx12Constants::Instance().GetMappedMatrix();
	m->world *= DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
	m->world *= DirectX::XMMatrixRotationY(angle.x);	// 回転
	m->world *= DirectX::XMMatrixRotationX(angle.y);
}

void Dx12Wrapper::Draw()
{
	// 命令のクリア
	ClearCmd(pipelineState, rootSignature);

	// ビューポートとシザー設定
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	auto dev = Dx12Device::Instance().GetDevice();
	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	bbIdx = swapChain->GetCurrentBackBufferIndex();		// バックバッファインデックスを調べる
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// クリアカラー設定
	cmdList->OMSetRenderTargets(1, &heapStart, false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());		// レンダーターゲット設定
	cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// 深度バッファのクリア

	// バリアの解除(ここから書き込みが始まる)
	UnlockBarrier(backBuffers[bbIdx]);

	// 画面のクリア(これも書き込みに入る)
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// クリア

	// モデルの描画
	pmdManager->Draw(cmdList);

	// バリアのセット
	SetBarrier();

	cmdList->Close();	// クローズ
	ExecuteCmd();
	WaitExecute();


	swapChain->Present(1, 0);	// 描画
}
