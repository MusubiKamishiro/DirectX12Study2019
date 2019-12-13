#include "Dx12Wrapper.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "d3dx12.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"

#include "Application.h"
#include "Dx12Device.h"
#include "Dx12Constants.h"
#include "PMDManager.h"
#include "VMDLoader.h"
#include "ImageManager.h"

#include "PrimitiveManager.h"
#include "Plane.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "Effekseer.lib")
#pragma comment(lib, "EffekseerRendererDX12.lib")
#pragma comment(lib, "LLGI.lib")


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
	// 各デスクリプターの使用するサイズを計算しとく
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < static_cast<int>(swapCDesc.BufferCount); ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));	// キャンバスを取得
		dev->CreateRenderTargetView(backBuffers[i], nullptr, cpuDescH);		// キャンバスと職人を紐づける
		cpuDescH.ptr += rtvSize;	// レンダーターゲットビューのサイズぶんずらす
	}
}

void Dx12Wrapper::InitShader()
{
	auto result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);

	InitRootSignature();
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

void Dx12Wrapper::InitRootSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE> descRange;
	descRange.resize(4);
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
	// "b2" 骨
	descRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[3].BaseShaderRegister = 2;	// レジスタ番号
	descRange[3].NumDescriptors = 1;		// 1回で読む数
	descRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	std::vector<D3D12_ROOT_PARAMETER> rootParam;
	rootParam.resize(3);
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;			// レンジの数
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];	// 対応するレンジへのポインタ
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;			// レンジの数
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];	// 対応するレンジへのポインタ
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;			// レンジの数
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[3];	// 対応するレンジへのポインタ
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignature = CreateRootSignature(rootSignature, rootParam, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
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
		{ "BONENO", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;		// このターゲット数と設定するフォーマット数は
	gpsDesc.NumRenderTargets = 3;							// 一致させておく

	// 深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;				// 必須(DSV)
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// DSV必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// 小さいほうを返す

	// ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 表面だけじゃなくて、裏面も描画するようにする

	// その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
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
	
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};	// 特に設定の必要はない
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

	// 深度バッファビューの作成
	dev->CreateDepthStencilView(depthBuff, &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::ClearCmd(ID3D12PipelineState* pipelinestate, ID3D12RootSignature* rootsignature)
{
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator, pipelinestate);
	cmdList->SetGraphicsRootSignature(rootsignature);
}

void Dx12Wrapper::Barrier(ID3D12Resource* buffer, const D3D12_RESOURCE_STATES& before, const D3D12_RESOURCE_STATES& after)
{
	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// バリア
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = buffer;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = before;
	BarrierDesc.Transition.StateAfter = after;
	
	// バリアを解除
	cmdList->ResourceBarrier(1, &BarrierDesc);
}

void Dx12Wrapper::ExecuteCmd()
{
	ID3D12CommandList* cmdLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
	cmdQueue->Signal(fence, ++fenceValue);
}

void Dx12Wrapper::WaitExecute()
{
	while (fence->GetCompletedValue() < fenceValue)
	{
		;// 待つだけなので何もしないよ
	}
}

void Dx12Wrapper::CreateShadowBuff()
{
	// 2の乗数のサイズ(正方形)にする
	auto wsize = Application::Instance().GetWindowSize();
	auto size = max(wsize.width, wsize.height);
	size_t bit = 0x8000000;
	for (size_t i = 31; i >= 0; --i)
	{
		if (size & bit)
		{
			break;
		}
		bit >>= 1;
	}
	
	D3D12_RESOURCE_DESC shadowDesc = {};
	shadowDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	shadowDesc.Width = size;
	shadowDesc.Height = size;
	shadowDesc.DepthOrArraySize = 1;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	shadowDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	shadowDesc.MipLevels = 1;
	shadowDesc.Alignment = 0;

	D3D12_CLEAR_VALUE shadowClearValue = {};
	shadowClearValue.DepthStencil.Depth = 1.0f;
	shadowClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	auto dev = Dx12Device::Instance().GetDevice();
	// 影バッファの作成
	auto result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
							&shadowDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &shadowClearValue, IID_PPV_ARGS(&shadowBuff));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&shadowDsvHeap));

	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&shadowSrvHeap));
	
	// 影用ビューポート、シザーレクトを設定
	shadowViewport.TopLeftX = 0;
	shadowViewport.TopLeftY = 0;
	shadowViewport.Width = size;
	shadowViewport.Height = size;
	shadowViewport.MaxDepth = 1.0f;
	shadowViewport.MinDepth = 0.0f;

	shadowScissorRect.left = 0;
	shadowScissorRect.top = 0;
	shadowScissorRect.right = size;
	shadowScissorRect.bottom = size;
}

void Dx12Wrapper::InitShadowShader()
{
	auto result = D3DCompileFromFile(L"LightShader.hlsl", nullptr, nullptr, "vs", "vs_5_0", 0, 0, &shadowVertexShader, nullptr);

	InitShadowRootSignature();
	InitShadowPipelineState();
}

void Dx12Wrapper::InitShadowRootSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE> shadowDescRange;
	shadowDescRange.resize(2);
	// "b0"	カメラ
	shadowDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	shadowDescRange[0].BaseShaderRegister = 0;
	shadowDescRange[0].NumDescriptors = 1;
	shadowDescRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "b1" 骨
	shadowDescRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	shadowDescRange[1].BaseShaderRegister = 1;
	shadowDescRange[1].NumDescriptors = 1;
	shadowDescRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


	std::vector<D3D12_ROOT_PARAMETER> shadowRootParam;
	shadowRootParam.resize(2);
	shadowRootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	shadowRootParam[0].DescriptorTable.pDescriptorRanges = &shadowDescRange[0];
	shadowRootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	shadowRootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	shadowRootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	shadowRootParam[1].DescriptorTable.pDescriptorRanges = &shadowDescRange[1];
	shadowRootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	shadowRootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	shadowRootSignature = CreateRootSignature(shadowRootSignature, shadowRootParam, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

void Dx12Wrapper::InitShadowPipelineState()
{
	D3D12_INPUT_ELEMENT_DESC shadowLayouts[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "BONENO", 0, DXGI_FORMAT_R16G16_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "WEIGHT", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
											D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowGPSDesc = {};
	shadowGPSDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// ルートシグネチャと頂点レイアウト
	shadowGPSDesc.pRootSignature = shadowRootSignature;
	shadowGPSDesc.InputLayout.pInputElementDescs = shadowLayouts;
	shadowGPSDesc.InputLayout.NumElements = _countof(shadowLayouts);
	// シェーダ系
	shadowGPSDesc.VS = CD3DX12_SHADER_BYTECODE(shadowVertexShader);
	// 深度ステンシル
	shadowGPSDesc.DepthStencilState.DepthEnable = true;
	shadowGPSDesc.DepthStencilState.StencilEnable = false;
	shadowGPSDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	shadowGPSDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowGPSDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	// ラスタライザ
	shadowGPSDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	shadowGPSDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// その他
	shadowGPSDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	shadowGPSDesc.NodeMask = 0;
	shadowGPSDesc.SampleDesc.Count = 1;		// いる
	shadowGPSDesc.SampleDesc.Quality = 0;	// いる
	shadowGPSDesc.SampleMask = 0xffffffff;	// 全部1
	shadowGPSDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形

	auto result = Dx12Device::Instance().GetDevice()->CreateGraphicsPipelineState(&shadowGPSDesc, IID_PPV_ARGS(&shadowPipelineState));
}

void Dx12Wrapper::CreateLightView()
{
	auto dev = Dx12Device::Instance().GetDevice();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dev->CreateDepthStencilView(shadowBuff, &dsvDesc, shadowDsvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dev->CreateShaderResourceView(shadowBuff, &srvDesc, shadowSrvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::CreateFirstPassBuff()
{
	auto dev = Dx12Device::Instance().GetDevice();

	// 1パス目のRTV,SRVの作成
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = 3;
	auto result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&heapFor1stPassRTV));

	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&heapFor1stPassSRV));

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	auto wsize = Application::Instance().GetWindowSize();
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0;
	resDesc.Width = wsize.width;
	resDesc.Height = wsize.height;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 0;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Color[0] = 1.0f;
	clearValue.Color[1] = 1.0f;
	clearValue.Color[2] = 1.0f;
	clearValue.Color[3] = 1.0f;
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	for (auto& buff : firstPassBuff)
	{
		result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, &clearValue, IID_PPV_ARGS(&buff));
	}
	for (auto& buff : bloomBuff)
	{
		result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&buff));

		resDesc.Width = resDesc.Width / 2;
		resDesc.Height = resDesc.Height / 2;
	}

	auto handle = heapFor1stPassRTV->GetCPUDescriptorHandleForHeapStart();
	// ビューの作成
	for (auto& buff : firstPassBuff)
	{
		dev->CreateRenderTargetView(buff, nullptr, handle);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}
	dev->CreateRenderTargetView(bloomBuff[0], nullptr, handle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle = heapFor1stPassSRV->GetCPUDescriptorHandleForHeapStart();
	for (auto& buff : firstPassBuff)
	{
		dev->CreateShaderResourceView(buff, &srvDesc, handle);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	dev->CreateShaderResourceView(bloomBuff[0], &srvDesc, handle);
}

void Dx12Wrapper::CreateScreenTexture()
{
	Vertex vertices[] = {
		// 正面
		{DirectX::XMFLOAT3(-1, -1, 0),	DirectX::XMFLOAT2(0, 1)},	// 左下
		{DirectX::XMFLOAT3(-1, 1, 0),	DirectX::XMFLOAT2(0, 0)},	// 左上
		{DirectX::XMFLOAT3(1, -1, 0),	DirectX::XMFLOAT2(1, 1)},	// 右下
		{DirectX::XMFLOAT3(1, 1, 0),	DirectX::XMFLOAT2(1, 0)},	// 右上
	};

	auto dev = Dx12Device::Instance().GetDevice();
	// 頂点バッファの作成
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&screenVertexBuffer));

	Vertex* vertexMap = nullptr;
	result = screenVertexBuffer->Map(0, nullptr, (void**)& vertexMap);
	std::copy(std::begin(vertices), std::end(vertices), vertexMap);
	screenVertexBuffer->Unmap(0, nullptr);

	svbView.BufferLocation = screenVertexBuffer->GetGPUVirtualAddress();
	svbView.StrideInBytes = sizeof(Vertex);	// 頂点1つあたりのバイト数
	svbView.SizeInBytes = sizeof(vertices);	// データ全体のサイズ
}

void Dx12Wrapper::InitLastRootSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE> descRange;
	descRange.resize(1);
	// "t0,t1,t2" テクスチャ, 法線情報, 高輝度成分
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[0].BaseShaderRegister = 0;	// レジスタ番号
	descRange[0].NumDescriptors = 3;		// 1回で読む数
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	std::vector<D3D12_ROOT_PARAMETER> rootParam;
	rootParam.resize(1);
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;			// レンジの数
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];	// 対応するレンジへのポインタ
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	lastRootSignature = CreateRootSignature(lastRootSignature, rootParam, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
}

void Dx12Wrapper::InitLastPipelineState()
{
	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC layouts[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// パイプラインステートを作る
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = lastRootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layouts;
	gpsDesc.InputLayout.NumElements = _countof(layouts);
	// シェーダ系
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(lastVertexShader);	// 頂点シェーダ
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(lastPixelShader);	// ピクセルシェーダ
	// レンダターゲット
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;		// このターゲット数と設定するフォーマット数は
	gpsDesc.NumRenderTargets = 1;							// 一致させておく
	// 深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	// ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// 表面だけじゃなくて、裏面も描画するようにする
	// その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;	// 全部1
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// 三角形

	auto result = Dx12Device::Instance().GetDevice()->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&lastPipelineState));
}

void Dx12Wrapper::InitLastShader()
{
	auto result = D3DCompileFromFile(L"LastShader.hlsl", nullptr, nullptr, "vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &lastVertexShader, nullptr);
	result = D3DCompileFromFile(L"LastShader.hlsl", nullptr, nullptr, "ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &lastPixelShader, nullptr);

	InitLastRootSignature();
	InitLastPipelineState();
}

ID3D12RootSignature* Dx12Wrapper::CreateRootSignature(ID3D12RootSignature* rootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParam, const D3D12_TEXTURE_ADDRESS_MODE& addressMode)
{
	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	// 特別なフィルタを使用しない
	samplerDesc.AddressU = addressMode;			// 絵が繰り返される(U方向)
	samplerDesc.AddressV = addressMode;			// 絵が繰り返される(V方向)
	samplerDesc.AddressW = addressMode;			// 絵が繰り返される(W方向)
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
	rsd.NumParameters = rootParam.size();
	rsd.NumStaticSamplers = 1;
	rsd.pParameters = &rootParam.front();
	rsd.pStaticSamplers = &samplerDesc;

	ID3DBlob* signature = nullptr;		// ルートシグネチャをつくるための材料
	ID3DBlob* error = nullptr;			// エラー出た時の対処
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ルートシグネチャの作成
	result = Dx12Device::Instance().GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	if (result == S_OK)
	{
		return rootSignature;
	}
	return nullptr;
}

void Dx12Wrapper::EffekseerInit()
{
	auto dev = Dx12Device::Instance().GetDevice();
	// Effekseerテスト
	auto format = DXGI_FORMAT_R8G8B8A8_UNORM;
	efkRenderer = EffekseerRendererDX12::Create(dev, cmdQueue, 2, &format, 1, false, false, 2000);	// デバイス, コマンドキュー, バックバッファ数, バックバッファのフォーマット, レンダーターゲット数, 深度値有効フラグ, 反転デプスフラグ, 最大パーティクル数
	efkManager = Effekseer::Manager::Create(2000);

	// 描画用インスタンスから描画機能を設定
	efkManager->SetSpriteRenderer(efkRenderer->CreateSpriteRenderer());
	efkManager->SetRibbonRenderer(efkRenderer->CreateRibbonRenderer());
	efkManager->SetTrackRenderer(efkRenderer->CreateTrackRenderer());
	efkManager->SetModelRenderer(efkRenderer->CreateModelRenderer());

	// 描画用インスタンスからテクスチャの読込機能を設定
	// 独自拡張機能、現在はファイルからの読込
	efkManager->SetTextureLoader(efkRenderer->CreateTextureLoader());
	efkManager->SetModelLoader(efkRenderer->CreateModelLoader());

	// エフェクト発生位置を設定
	auto efkPos = Effekseer::Vector3D(0.0f, 0.0f, 0.0f);

	// メモリプール
	efkMemoryPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(efkRenderer);
	// コマンドリスト作成
	efkCmdList = EffekseerRendererDX12::CreateCommandList(efkRenderer, efkMemoryPool);
	// コマンドリストセット
	efkRenderer->SetCommandList(efkCmdList);

	// 投影行列を設定
	auto wsize = Application::Instance().GetWindowSize();	// 画面サイズ
	auto aspect = (float)wsize.width / (float)wsize.height;	// ビュー空間の高さと幅のアスペクト比
	efkRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovLH(90.f / 180.0f * 3.14f, aspect, 1.0f, 100.0f));

	// カメラ行列を設定
	efkRenderer->SetCameraMatrix(Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(0.0f, 10.0f, -25.0f), Effekseer::Vector3D(0.0f, 15.0f, 0.0f), Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));

	// エフェクトの読込
	effect = Effekseer::Effect::Create(efkManager, (const EFK_CHAR*)L"effect/test.efk");
	//effect = Effekseer::Effect::Create(efkManager, (const EFK_CHAR*)L"effect/blood/BloodLance.efk");

}

void Dx12Wrapper::ImGuiInit(HWND hwnd)
{
	auto dev = Dx12Device::Instance().GetDevice();

	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// ヒープ作成
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&imguiHeap));

	// 初期化
	ImGui::CreateContext();

	bool imguiResult = ImGui_ImplWin32_Init(hwnd);
	imguiResult = ImGui_ImplDX12_Init(dev, 2, DXGI_FORMAT_R8G8B8A8_UNORM, imguiHeap, 
		imguiHeap->GetCPUDescriptorHandleForHeapStart(), imguiHeap->GetGPUDescriptorHandleForHeapStart());
}

void Dx12Wrapper::ImGuiDraw()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(400, 300));
	ImGui::Begin("test");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Separator();		//区切り線
	ImGui::Checkbox("Motion Play", &motionPlayFlag);
	ImGui::SameLine();		// 改行のキャンセル
	ImGui::Checkbox("Motion ReversePlay", &motionReversePlayFlag);
	ImGui::SliderInt("Now Frame", &frame, 0, maxFrame);
	ImGui::Text("Camera Chapter");
	ImGui::SameLine();
	if (ImGui::Button("before"))
	{
		frame = vmdCamera->GetCameraBeforePeriod(frame);
	}
	ImGui::SameLine();
	if (ImGui::Button("next"))
	{
		frame = vmdCamera->GetCameraNextPeriod(frame);
	}
	auto eye = Dx12Constants::Instance().GetEyePos();
	ImGui::Text("CameraPos x:%.3f, y:%.3f, z:%.3f", eye.x, eye.y, eye.z);
	auto focus = Dx12Constants::Instance().GetFocusPos();
	ImGui::Text("FocusPos x:%.3f, y:%.3f, z:%.3f", focus.x, focus.y, focus.z);
	auto rotation = Dx12Constants::Instance().GetRotation();
	ImGui::Text("rotation x:%.3f, y:%.3f, z:%.3f", rotation.x, rotation.y, rotation.z);
	for (int i = 0; i < pmdManagers.size(); ++i)
	{
		auto modelPos = pmdManagers[i]->GetPos();
		std::string s = "modelPos[" + std::to_string(i) + "] x:%.3f, y:%.3f, z:%.3f";
		ImGui::Text(s.c_str(), modelPos.x, modelPos.y, modelPos.z);
	}
	//ImGui::ColorPicker4("ColorPicker4", clearColor);
	ImGui::End();
	ImGui::Render();
	cmdList->SetDescriptorHeaps(1, &imguiHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
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
	
	// PMDモデルの読み込み
	//pmdManagers.emplace_back(new PMDManager("model/vocaloid/巡音ルカ.pmd");
	//pmdManagers.emplace_back(new PMDManager("model/hibiki/我那覇響v1_グラビアミズギ.pmd"));
	//pmdManagers.emplace_back(new PMDManager("model/yayoi/やよいヘッド_カジュアル（体x0.96）改造.pmd"));
	pmdManagers.emplace_back(new PMDManager("model/vocaloid/初音ミク.pmd"));
	//pmdManagers.emplace_back(new PMDManager("model/vocaloid/鏡音レン.pmd"));
	pmdManagers.emplace_back(new PMDManager("model/hibiki/我那覇響v1_グラビアミズギ.pmd"));
	pmdManagers.emplace_back(new PMDManager("model/vocaloid/巡音ルカ.pmd"));
	pmdManagers.emplace_back(new PMDManager("model/vocaloid/鏡音リン.pmd"));
	pmdManagers.emplace_back(new PMDManager("model/vocaloid/弱音ハク.pmd"));

	// VMDの読み込み
	//vmdLoaders.emplace_back(new VMDLoader(motion/ヤゴコロダンス.vmd");
	//vmdLoaders.emplace_back(new VMDLoader("motion/PBA_Solo.vmd");// Princess Be Ambitious!!
	vmdLoaders.emplace_back(new VMDLoader("motion/KimagureMercy/Tda/Miku.vmd"));
	vmdLoaders.emplace_back(new VMDLoader("motion/KimagureMercy/Tda/Teto.vmd"));
	vmdLoaders.emplace_back(new VMDLoader("motion/KimagureMercy/Tda/Luka.vmd"));
	vmdLoaders.emplace_back(new VMDLoader("motion/KimagureMercy/Tda/Rin.vmd"));
	vmdLoaders.emplace_back(new VMDLoader("motion/KimagureMercy/Tda/Haku.vmd"));

	// VMD(カメラ)の読み込み
	vmdCamera.reset(new VMDLoader("motion/KimagureMercy/camera.vmd"));

	CreateDepthBuff();
	
	CreateFirstPassBuff();
	CreateScreenTexture();

	// 床
	primitiveManager.reset(new PrimitiveManager());
	//plane.reset(primitiveManager->CreatePlane(DirectX::XMFLOAT3(0, 0, 0), 200, 200));
	plane.reset(primitiveManager->CreatePlane(DirectX::XMFLOAT3(0, 0, 0), 0, 0));
	// 床に張る画像
	imageManager.reset(new ImageManager());
	floorImgBuff = imageManager->Load("img/masaki.png");
	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	// ヒープ作成
	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&floorImgHeap));
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dev->CreateShaderResourceView(floorImgBuff, &srvDesc, floorImgHeap->GetCPUDescriptorHandleForHeapStart());

	InitShader();
	InitLastShader();

	// 影用
	CreateShadowBuff();
	InitShadowShader();
	CreateLightView();

	cmdList->Close();

	startTime = GetTickCount64();
	maxFrame = vmdLoaders[0]->GetMaxFrame();

	EffekseerInit();

	// imgui関係
	ImGuiInit(hwnd);
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
	heapFor1stPassRTV->Release();
	heapFor1stPassSRV->Release();
	for (auto& buff : firstPassBuff)
	{
		buff->Release();
	}
	lastPipelineState->Release();
	lastRootSignature->Release();

	screenVertexBuffer->Release();
	for (auto& backBuffer : backBuffers)
	{
		backBuffer->Release();
	}
	for (auto& buff : bloomBuff)
	{
		buff->Release();
	}

	depthBuff->Release();
	dsvHeap->Release();

	shadowBuff->Release();
	shadowDsvHeap->Release();
	shadowSrvHeap->Release();
	shadowRootSignature->Release();
	shadowPipelineState->Release();
	shadowVertexShader->Release();

	floorImgHeap->Release();

	efkRenderer->Release();
	efkManager->Release();
	efkMemoryPool->Release();
	efkCmdList->Release();
	effect->UnloadResources();
	effect->Release();

	imguiHeap->Release();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Dx12Wrapper::Update()
{
	// 入力関連
	unsigned char keyState[256] = {};

	Vector3 pos = {};
	Vector3 angle = {};

	if (GetKeyboardState(keyState))
	{
		// Effekseer
		if (keyState[VK_SPACE] & 0x80)
		{
			if (efkManager->Exists(efkHandle))
			{
				efkManager->StopEffect(efkHandle);
			}
			efkHandle = efkManager->Play(effect, Effekseer::Vector3D(0, 0, 0));
		}
	}
	
	Dx12Constants::Instance().Update(vmdCamera->GetCameraData(), frame);


	if (motionPlayFlag)
	{
		++frame;
	}
	if (motionReversePlayFlag)
	{
		--frame;
	}
	if (frame > maxFrame)
	{
		frame = 0;
	}
	else if (frame < 0)
	{
		frame = maxFrame;
	}

	for (int i = 0; i < pmdManagers.size(); ++i)
	{
		pmdManagers[i]->Update(vmdLoaders[i]->GetAnimationData(), vmdLoaders[i]->GetSkinData(), frame);
	}
}

void Dx12Wrapper::Draw()
{
	/// 影の描画 ///
	// 命令のクリア
	ClearCmd(shadowPipelineState, shadowRootSignature);

	// ライトからの深度を描く
	cmdList->RSSetViewports(1, &shadowViewport);
	cmdList->RSSetScissorRects(1, &shadowScissorRect);

	auto sdsvh = shadowDsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->OMSetRenderTargets(0, nullptr, false, &sdsvh);
	cmdList->ClearDepthStencilView(sdsvh, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// バリアを解除
	Barrier(shadowBuff, D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	for (auto& pmd : pmdManagers)
	{
		pmd->ShadowDraw(cmdList);
	}

	// バリアを張る
	Barrier(shadowBuff, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT);

	cmdList->Close();
	ExecuteCmd();
	WaitExecute();


	/// モデルの描画 ///
	// 命令のクリア
	ClearCmd(pipelineState, rootSignature);
	// ビューポートとシザー設定
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	auto dev = Dx12Device::Instance().GetDevice();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heapFor1stPassRTV->GetCPUDescriptorHandleForHeapStart();
	auto incSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 3> rtvs;
	for (auto& rtv : rtvs)
	{
		rtv = handle;
		handle.ptr += incSize;
	}
	cmdList->OMSetRenderTargets(3, rtvs.data(), false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());		// レンダーターゲット設定
	cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// 深度バッファのクリア

	// バリアの解除(ここから書き込みが始まる)
	for (auto& buff : firstPassBuff)
	{
		Barrier(buff, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	for (auto& buff : bloomBuff)
	{
		Barrier(buff, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	// 画面のクリア(これも書き込みに入る)
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// クリアカラー設定
	for (auto& rtv : rtvs)
	{
		cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
	// モデルの描画
	for (auto& pmd : pmdManagers)
	{
		pmd->Draw(cmdList);
	}
	// 床の描画
	primitiveManager->SetPrimitiveDrawMode(cmdList);
	auto rgst = Dx12Constants::Instance().GetRgstDescriptorHeap();
	cmdList->SetDescriptorHeaps(1, &rgst);
	cmdList->SetGraphicsRootDescriptorTable(0, rgst->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetDescriptorHeaps(1, &shadowSrvHeap);
	cmdList->SetGraphicsRootDescriptorTable(1, shadowSrvHeap->GetGPUDescriptorHandleForHeapStart());
	cmdList->SetDescriptorHeaps(1, &floorImgHeap);
	cmdList->SetGraphicsRootDescriptorTable(2, floorImgHeap->GetGPUDescriptorHandleForHeapStart());
	plane->Draw(cmdList);

	// エフェクト描画
	efkManager->Update();
	efkMemoryPool->NewFrame();
	EffekseerRendererDX12::BeginCommandList(efkCmdList, cmdList);
	efkRenderer->BeginRendering();
	efkManager->Draw();
	efkRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(efkCmdList);
	
	// バリアのセット
	for (auto& buff : firstPassBuff)
	{
		Barrier(buff, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
	for (auto& buff : bloomBuff)
	{
		Barrier(buff, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}

	cmdList->Close();	// クローズ
	ExecuteCmd();
	WaitExecute();


	/// 最終描画 ///
	// 命令のクリア
	ClearCmd(lastPipelineState, lastRootSignature);
	// ビューポートとシザー設定
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	bbIdx = swapChain->GetCurrentBackBufferIndex();		// バックバッファインデックスを調べる
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);		// レンダーターゲット設定
	cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// 深度バッファのクリア

	// バリアの解除(ここから書き込みが始まる)
	Barrier(backBuffers[bbIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// 画面のクリア(これも書き込みに入る)
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	cmdList->SetDescriptorHeaps(1, &heapFor1stPassSRV);
	cmdList->SetGraphicsRootDescriptorTable(0, heapFor1stPassSRV->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->IASetVertexBuffers(0, 1, &svbView);
	// ペラポリ描画
	cmdList->DrawInstanced(4, 1, 0, 0);

	// imgui描画
	ImGuiDraw();
	
	// バリアのセット
	Barrier(backBuffers[bbIdx], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	cmdList->Close();	// クローズ
	ExecuteCmd();
	WaitExecute();


	swapChain->Present(1, 0);	// 描画
}
