#include "Dx12Wrapper.h"
#include <d3dcompiler.h>

#include "Application.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

void Dx12Wrapper::CreateDebugLayer(HRESULT& result)
{
	ID3D12Debug* debug;
	result = D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
	if (SUCCEEDED(result))
	{
		debug->EnableDebugLayer();
	}
	debug->Release();
}

void Dx12Wrapper::InitFeatureLevel(HRESULT& result)
{
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// ���x���̍������̂��猟�؂��A�����������x����K�p����
	for (auto& l : levels)
	{
		// �f�B�X�v���C�A�_�v�^�[��\���f�o�C�X�̍쐬
		result = D3D12CreateDevice(nullptr, l, IID_PPV_ARGS(&dev));

		if (SUCCEEDED(result))
		{
			break;
		}
	}
}

void Dx12Wrapper::CreateSwapChain(HRESULT& result, HWND hwnd)
{
	// DXGI�t�@�N�g���̍쐬
	result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	// �R�}���h�L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&cmdQueue));

	// �X���b�v�`�F�C���̍쐬
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
	swapChainDesc.BufferCount = 2;			// �o�b�N�o�b�t�@�̐�

	result = dxgiFactory->CreateSwapChainForHwnd(
		cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)(&swapChain));
}

void Dx12Wrapper::CreateRenderTarget(HRESULT& result)
{
	// �����_�[�^�[�Q�b�g�쐬�̂��߂̑O����
	// �\����ʗp�������m��
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �����_�[�^�[�Q�b�g�r���[
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = 2;	// �\��ʂƗ����
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));


	DXGI_SWAP_CHAIN_DESC swapCDesc = {};
	swapChain->GetDesc(&swapCDesc);
	// �����_�[�^�[�Q�b�g���Ԃ�m��
	backBuffers.resize(swapCDesc.BufferCount);
	// �����_�[�^�[�Q�b�g�̍쐬
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// �e�޽�������̎g�p���黲�ނ��v�Z���Ƃ�
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < static_cast<int>(swapCDesc.BufferCount); ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));	// �L�����o�X���擾
		dev->CreateRenderTargetView(backBuffers[i], nullptr, cpuDescH);		// �L�����o�X�ƐE�l��R�Â���
		cpuDescH.ptr += rtvSize;	// �����_�[�^�[�Q�b�g�r���[�̃T�C�Y�Ԃ񂸂炷
	}
}

void Dx12Wrapper::CreateVertexBuffer(HRESULT& result)
{
	DirectX::XMFLOAT3 vertices[] = { {-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f} };

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);	// ���_��񂪓��邾���̃T�C�Y
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// ���_�o�b�t�@�̍쐬
	result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc,
								D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	D3D12_RANGE range = { 0,0 };
	DirectX::XMFLOAT3* vertexMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)& vertexMap);
	std::copy(std::begin(vertices), std::end(vertices), vertexMap);
	vertexBuffer->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(Vertex);	// ���_1������̃o�C�g��
	vbView.SizeInBytes = sizeof(vertices);	// �f�[�^�S�̂̃T�C�Y

	cmdList->IASetVertexBuffers(0, 1, &vbView);
}

void Dx12Wrapper::InitScreen()
{
	// ��ʂ̃N���A
	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	bbIdx = swapChain->GetCurrentBackBufferIndex();		// �ޯ��ޯ̧���ޯ���𒲂ׂ�
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };				// �N���A�J���[�ݒ�
	cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);		// �����_�[�^�[�Q�b�g�ݒ�
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// �N���A

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	cmdList->Close();
}

void Dx12Wrapper::InitShader(HRESULT& result)
{
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicVS", "vs_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "BasicPS", "ps_5_0",
						D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;

	InitRootSignatur(result);
	InitPipelineState(result);
}

void Dx12Wrapper::InitRootSignatur(HRESULT& result)
{
	ID3DBlob* signature = nullptr;		// ���[�g�V�O�l�`�������邽�߂̍ޗ�
	ID3DBlob* error = nullptr;			// �G���[�o�����̑Ώ�

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ���[�g�V�O�l�`���̍쐬
	result = dev->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
																				IID_PPV_ARGS(&rootSignature));
}

void Dx12Wrapper::InitPipelineState(HRESULT& result)
{
	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC layouts[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
														D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

	// �p�C�v���C���X�e�[�g�����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	//gpsDesc = CD3D12_BLEND_DESC(D3D12_DEFAULT);

	// ���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layouts;
	gpsDesc.InputLayout.NumElements = _countof(layouts);

	// �V�F�[�_�n
	//gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);		// ���_�V�F�[�_
	//gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);		// �s�N�Z���V�F�[�_

	// �����_�^�[�Q�b�g
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;		// �������ޯĐ��Ɛݒ肷��t�H�[�}�b�g����
	gpsDesc.NumRenderTargets = 1;							// ��v�����Ă���

	// �[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;			// ���Ƃ�
	gpsDesc.DepthStencilState.StencilEnable = false;		// ���Ƃ�
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;				// �K�{
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// ���X�^���C�U
	//gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �\�ʂ�������Ȃ��āA���ʂ��`�悷��悤�ɂ����

	// ���̑�
	//gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;		// ����
	gpsDesc.SampleDesc.Quality = 0;		// ����
	gpsDesc.SampleMask = 0xffffffff;	// �S��1
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`

	result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
}

void Dx12Wrapper::ExecuteCmd()
{
	ID3D12CommandList* cmdLists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdLists);
	cmdQueue->Signal(fence, ++fenceValue);
}

void Dx12Wrapper::WaitExecute()
{
	while (fence->GetCompletedValue() != fenceValue)
	{
		;// �҂����Ȃ̂ŉ������Ȃ���
	}
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
{
	HRESULT result = S_OK;

#ifdef _DEBUG
	CreateDebugLayer(result);
#endif // _DEBUG

	InitFeatureLevel(result);
	CreateSwapChain(result, hwnd);
	CreateRenderTarget(result);
	
	// ����ޱ۹���ƺ����ؽĂ̐���
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
	// �t�F���X�̍쐬
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	// ���߂��ĂԑO�Ƀ��Z�b�g
	cmdAllocator->Reset();
	cmdList->Reset(cmdAllocator, nullptr);

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	CreateVertexBuffer(result);

	InitScreen();
	InitShader(result);
	ExecuteCmd();
	WaitExecute();

	swapChain->Present(0, 0);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &BarrierDesc);
}


Dx12Wrapper::~Dx12Wrapper()
{
}

void Dx12Wrapper::Update()
{
}

void Dx12Wrapper::Draw()
{
}
