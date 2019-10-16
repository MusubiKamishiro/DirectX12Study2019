#include "Dx12Wrapper.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "d3dx12.h"
#include "shlwapi.h"

#include "Application.h"
#include "Dx12Device.h"
#include "PMDLoader.h"
#include "ImageManager.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "shlwapi.lib")


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

ID3D12Resource* Dx12Wrapper::CreateTextureResource(ID3D12Resource* buff, const unsigned int width, const unsigned int height, const unsigned int arraySize)
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0;
	resDesc.Width = width;
	resDesc.Height = height;
	resDesc.DepthOrArraySize = arraySize;
	resDesc.MipLevels = 0;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	auto result = Dx12Device::Instance().GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&buff));

	if (result == S_OK)
	{
		return buff;
	}
	return nullptr;
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

std::string Dx12Wrapper::GetExtension(const char* path)
{
	std::string s = path;
	size_t dpoint = s.rfind(".") + 1;		// "."の場所を探る
	return s.substr(dpoint);
}

std::pair<std::string, std::string> Dx12Wrapper::SplitFileName(const std::string& path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

void Dx12Wrapper::InitConstants()
{
	auto wsize = Application::Instance().GetWindowSize();	// 画面サイズ

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	mappedMatrix.world = world;

	// カメラの設定
	auto eyePos = DirectX::XMFLOAT3(0, 20, -15);	// カメラの位置(視点)
	auto focusPos = DirectX::XMFLOAT3(0, 10, 0);	// 焦点の位置(注視点)
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// カメラの上方向(通常は(0.0f, 1.0f, 0.0f))	// カメラを固定するためのもの

	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));	// カメラ行列
													// XMLoadFloat3...XMFloat3をXMVECTORに変換する

	auto aspect = (float)wsize.width / (float)wsize.height;		// ビュー空間の高さと幅のアスペクト比
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(3.1415f / 2.0f, aspect, 0.5f, 300.0f);		// 射影行列	// LH...LeftHandの略,RHもあるよ
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicLH(30, 30, 0.5f, 300.0f);

	mappedMatrix.viewProj = camera * projection;	// かける順番には気を付けよう
	mappedMatrix.wvp = world * camera * projection;

	auto lightPos = DirectX::XMFLOAT3(50, 70, -15);
	DirectX::XMMATRIX _lcamera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));
	mappedMatrix.lightVP = _lcamera * lightProj;

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256アライメントに合わせている

	auto dev = Dx12Device::Instance().GetDevice();
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));

	result = constBuff->Map(0, nullptr, (void**)&m);	// シェーダに送る
	*m = mappedMatrix;

	auto handle = rgstDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;
	dev->CreateConstantBufferView(&cbvDesc, handle);
}

void Dx12Wrapper::InitMaterials()
{
	// バッファのリサイズ
	materialBuffs.resize(pmdLoader->GetMatDatas().size());

	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = materialBuffs.size() * 5;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto dev = Dx12Device::Instance().GetDevice();
	// ヒープ作成
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&matDescriptorHeap));

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256アライメントに合わせている

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = size;

	// ハンドルの取得
	auto handle = matDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// マテリアル数分作る
	for (int i = 0; i < materialBuffs.size(); ++i)
	{
		// リソース作成
		result = dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&materialBuffs[i]));

		PMDMaterialData* matMap = nullptr;
		// マップする
		result = materialBuffs[i]->Map(0, nullptr, (void**)& matMap);
		// 着色
		matMap->diffuseColor = pmdLoader->GetMatDatas()[i].diffuseColor;		// 減光色
		matMap->specularColor = pmdLoader->GetMatDatas()[i].specularColor;	// 光沢色
		matMap->mirrorColor = pmdLoader->GetMatDatas()[i].mirrorColor;		// 環境色

		// アンマップ
		materialBuffs[i]->Unmap(0, nullptr);
		cbvDesc.BufferLocation = materialBuffs[i]->GetGPUVirtualAddress();

		auto hptr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// テクスチャがなければ、作った白テクスチャを使う。あれば、それを使う
		auto image = whiteTexBuff;
		auto spa = blackTexBuff;
		auto sph = whiteTexBuff;
		if (strlen(pmdLoader->GetModelTexturesPath()[i].c_str()) > 0)
		{
			// とりあえず目が見たいから仮実装
			std::string texFileName = pmdLoader->GetModelTexturesPath()[i];
			if (count(texFileName.begin(), texFileName.end(), '*') > 0)
			{
				auto namepair = SplitFileName(texFileName);
				if (GetExtension(namepair.first.c_str()) == "sph" || GetExtension(namepair.first.c_str()) == "spa")
				{
					texFileName = namepair.second;
				}
				else
				{
					texFileName = namepair.first;
				}
			}
			auto ext = GetExtension(texFileName.c_str());
			// ここまで仮実装

			if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "tga")
			{
				image = modelTexBuff[i];
			}
			else if (ext == "spa")
			{
				spa = spaBuff[i];
			}
			else if (ext == "sph")
			{
				sph = sphBuff[i];
			}
		}

		// トゥーンがあればそれを、なければデフォルトを使う
		auto toon = gradTexBuff;
		if (pmdLoader->GetMatDatas()[i].toonIndex != 0xff)
		{
			toon = toonBuff[pmdLoader->GetMatDatas()[i].toonIndex];
		}

		// マテリアルの色
		dev->CreateConstantBufferView(&cbvDesc, handle);
		handle.ptr += hptr;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// 通常テクスチャ
		dev->CreateShaderResourceView(image, &srvDesc, handle);
		handle.ptr += hptr;

		// 加算(spa)
		dev->CreateShaderResourceView(spa, &srvDesc, handle);
		handle.ptr += hptr;

		// 乗算(sph)
		dev->CreateShaderResourceView(sph, &srvDesc, handle);
		handle.ptr += hptr;

		// トゥーン(toon)
		dev->CreateShaderResourceView(toon, &srvDesc, handle);
		handle.ptr += hptr;
	}
}

void Dx12Wrapper::CreateModelTexture()
{
	unsigned int matNum = pmdLoader->GetMatDatas().size();

	modelTexBuff.resize(matNum);
	spaBuff.resize(matNum);
	sphBuff.resize(matNum);

	for (unsigned int i = 0; i < matNum; ++i)
	{
		if (std::strlen(pmdLoader->GetModelTexturesPath()[i].c_str()) == 0)
		{
			// パスがなければ画像はないのでやらなくてよし
			continue;
		}

		// スプリッタがある
		std::string texFileName = pmdLoader->GetModelTexturesPath()[i];
		if (count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first.c_str()) == "sph" || GetExtension(namepair.first.c_str()) == "spa")
			{
				texFileName = namepair.second;
			}
			else
			{
				texFileName = namepair.first;
			}
		}

		auto ext = GetExtension(texFileName.c_str());
		auto path = GetWideStringFromString(texFileName);

		// 画像読み込み&書き込み
		if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "tga")
		{
			modelTexBuff[i] = imageManager->Load(texFileName);
		}
		else if (ext == "spa")
		{
			spaBuff[i] = imageManager->Load(texFileName);
		}
		else if (ext == "sph")
		{
			sphBuff[i] = imageManager->Load(texFileName);
		}
	}
}

void Dx12Wrapper::CreateWhiteTexture()
{
	whiteTexBuff = CreateTextureResource(whiteTexBuff);

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);	// 白

	auto result = whiteTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void Dx12Wrapper::CreateBlackTexture()
{
	blackTexBuff = CreateTextureResource(blackTexBuff);

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);	// 黒

	auto result = blackTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void Dx12Wrapper::CreateGraduationTextureBuffer()
{
	gradTexBuff = CreateTextureResource(gradTexBuff);

	struct Color
	{
		Color() : r(0), g(0), b(0), a(0) {}
		Color(unsigned char inr, unsigned char ing, unsigned char inb, unsigned char ina)
			: r(inr), g(ing), b(inb), a(ina) {}

		unsigned char r, g, b, a;
	};

	std::vector<Color> data;
	data.resize(4 * 256);
	unsigned char brightness = 255;
	for (auto it = data.begin(); it != data.end(); it += 4)
	{
		std::fill_n(it, 4, Color(brightness, brightness, brightness, 0xff));
		--brightness;
	}

	auto result = gradTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(Color), data.size() * sizeof(Color));
}

void Dx12Wrapper::CreateToonTexture()
{
	std::vector<DirectX::TexMetadata> metadata = {};
	std::vector<DirectX::ScratchImage> toon;

	toonBuff.resize(10);
	metadata.resize(10);
	toon.resize(10);

	// トゥーン読み込み&書き込み
	for (int i = 0; i < toon.size(); i++)
	{
		size_t spoint = modelPath.rfind("/");		// "/"の場所を探る
		std::string modelToon = modelPath.substr(0, spoint + 1);
		std::string s = GetToonPathFromIndex(modelToon, i);
		auto path = GetWideStringFromString(s);
		
		// 読み込み
		auto result = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, &metadata[i], toon[i]);

		toonBuff[i] = CreateTextureResource(toonBuff[i], metadata[i].width, metadata[i].height, metadata[i].arraySize);

		// テクスチャ書き込み
		result = toonBuff[i]->WriteToSubresource(
			0,
			nullptr,
			toon[i].GetPixels(),
			metadata[i].width * 4,
			toon[i].GetPixelsSize());

		// 書き込んだら用済みなので解放
		toon[i].Release();
	}
}

std::string Dx12Wrapper::GetToonPathFromIndex(const std::string& folder, int idx)
{
	std::string filename = pmdLoader->GetToonTexNames()[idx];
	std::string path = "toon/";
	path += filename;

	// ファイルシステムオブジェクトへのパスが有効かどうかを判断する
	if (PathFileExistsA(path.c_str()))
	{
		return path;
	}
	else
	{
		return folder + filename;
	}
}

std::wstring Dx12Wrapper::GetWideStringFromString(std::string& str)
{
	// 呼び出し1回目(文字列数を得る)
	auto bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, nullptr, 0);

	// stringのwchar_t版, 得られた文字列数でリサイズしておく
	std::wstring wstr;
	wstr.resize(bsize);

	// 呼び出し2回目(確保済のwstrに変換文字列をコピー)
	bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, &wstr[0], bsize);

	return wstr;
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

	result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&rgstDescriptorHeap));

	imageManager.reset(new ImageManager());

	//modelPath = "model/vocaloid/初音ミク.pmd";
	modelPath = "model/vocaloid/初音ミクmetal.pmd";
	//modelPath = "model/vocaloid/巡音ルカ.pmd";
	//modelPath = "model/yayoi/やよいヘッド_カジュアル（体x0.96）改造.pmd";
	//modelPath = "model/hibiki/我那覇響v1.pmd";
	pmdLoader.reset(new PMDLoader(modelPath));

	CreateModelTexture();
	CreateWhiteTexture();
	CreateBlackTexture();
	CreateGraduationTextureBuffer();
	CreateToonTexture();
	InitMaterials();
	CreateDepthBuff();
	
	CreateVertexBuffer();

	InitShader();


	InitConstants();

	cmdList->Close();
}

Dx12Wrapper::~Dx12Wrapper()
{
	rgstDescriptorHeap->Release();
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

	constBuff->Release();
	depthBuff->Release();
	dsvHeap->Release();
	matDescriptorHeap->Release();

	for (int i = 0; i < materialBuffs.size(); ++i)
	{
		materialBuffs[i]->Release();
		if (modelTexBuff[i] != nullptr)
		{
			modelTexBuff[i]->Release();
		}
		if (spaBuff[i] != nullptr)
		{
			spaBuff[i]->Release();
		}
		if (sphBuff[i] != nullptr)
		{
			sphBuff[i]->Release();
		}
	}
	whiteTexBuff->Release();
	blackTexBuff->Release();
	gradTexBuff->Release();
	for (auto& toon : toonBuff)
	{
		toon->Release();
	}
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
	bbIdx = swapChain->GetCurrentBackBufferIndex();		// ﾊﾞｯｸﾊﾞｯﾌｧｲﾝﾃﾞｯｽｸを調べる
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// クリアカラー設定
	cmdList->OMSetRenderTargets(1, &heapStart, false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());		// レンダーターゲット設定
	cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// 深度バッファのクリア

	// バリアの解除(ここから書き込みが始まる)
	UnlockBarrier(backBuffers[bbIdx]);

	// 画面のクリア(これも書き込みに入る)
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// クリア

	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &pmdLoader->GetVbView());
	cmdList->IASetIndexBuffer(&pmdLoader->GetIbView());


	// モデルの描画
	unsigned int offset = 0;
	cmdList->SetDescriptorHeaps(1, &matDescriptorHeap);
	auto handle = matDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto hptr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto& mat : pmdLoader->GetMatDatas())
	{
		cmdList->SetGraphicsRootDescriptorTable(1, handle);
		handle.ptr += hptr * 5;
		cmdList->DrawIndexedInstanced(mat.faceVertCount, 1, offset, 0, 0);
		offset += mat.faceVertCount;
	}


	// バリアのセット
	SetBarrier();

	cmdList->Close();	// クローズ
	ExecuteCmd();
	WaitExecute();


	swapChain->Present(1, 0);	// 描画
}
