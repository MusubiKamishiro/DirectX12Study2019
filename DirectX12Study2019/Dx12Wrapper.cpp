#include "Dx12Wrapper.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "d3dx12.h"
#include "shlwapi.h"

#include "Application.h"
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

void Dx12Wrapper::InitFeatureLevel()
{
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// �O���t�B�b�N�X�A�_�v�^��񋓂�����
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(adapter);
	}
	// ���̒�����NVIDIA��T��
	for (auto& adpt : adapters)
	{
		DXGI_ADAPTER_DESC aDesc = {};
		adpt->GetDesc(&aDesc);
		std::wstring strDesc = aDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			// NVIDIA�A�_�v�^������
			adapter = adpt;
			break;
		}
	}

	// ���x���̍������̂��猟�؂��A�����������x����K�p����
	for (auto& l : levels)
	{
		// �f�B�X�v���C�A�_�v�^�[��\���f�o�C�X�̍쐬
		auto result = D3D12CreateDevice(adapter, l, IID_PPV_ARGS(&dev));

		if (SUCCEEDED(result))
		{
			break;
		}
	}
}

void Dx12Wrapper::CreateSwapChain(HWND hwnd)
{
	// �R�}���h�L���[�̍쐬
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	auto result = dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&cmdQueue));

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

void Dx12Wrapper::CreateRenderTarget()
{
	// �����_�[�^�[�Q�b�g�쐬�̂��߂̑O����
	// �\����ʗp�������m��
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �����_�[�^�[�Q�b�g�r���[
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.NumDescriptors = 2;	// �\��ʂƗ����
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	auto result = dev->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));


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

void Dx12Wrapper::CreateVertexBuffer()
{
	std::vector<Vertex> vertices;
	vertices.resize(4);
	vertices.emplace_back(DirectX::XMFLOAT3(-0.8f, -0.8f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f));	// ����
	vertices.emplace_back(DirectX::XMFLOAT3(-0.8f, 0.8f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f));		// ����
	vertices.emplace_back(DirectX::XMFLOAT3(0.8f, -0.8f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));		// �E��
	vertices.emplace_back(DirectX::XMFLOAT3(0.8f, 0.8f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f));		// �E��

	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = vertices.size();	// ���_��񂪓��邾���̃T�C�Y
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// ���_�o�b�t�@�̍쐬
	auto result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc,
								D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	std::vector<unsigned short> indices = { 0,1,2, 1,2,3 };	// ���_���g�p���鏇��
	// �C���f�b�N�X�o�b�t�@�̍쐬
	result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	D3D12_RANGE range = { 0,0 };
	Vertex* vertexMap = nullptr;
	result = vertexBuffer->Map(0, nullptr, (void**)& vertexMap);
	std::copy(std::begin(vertices), std::end(vertices), vertexMap);
	vertexBuffer->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(Vertex);	// ���_1������̃o�C�g��
	vbView.SizeInBytes = sizeof(vertices);	// �f�[�^�S�̂̃T�C�Y

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	// �o�b�t�@�̏ꏊ
	ibView.Format = DXGI_FORMAT_R16_UINT;	// �t�H�[�}�b�g(short�Ȃ̂�R16)
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);		// ���T�C�Y
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
	// �r���[�|�[�g�ݒ�
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(wsize.width);
	viewport.Height = static_cast<FLOAT>(wsize.height);
	viewport.MaxDepth = 1.0f;	// �J��������̋���(�����ق�)
	viewport.MinDepth = 0.0f;	// �J��������̋���(�߂��ق�)

	// �V�U�[(�؂���)��`�ݒ�
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
	descRange[0].BaseShaderRegister = 0;	// ���W�X�^�ԍ�
	descRange[0].NumDescriptors = 1;		// 1��œǂސ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "b1"	�}�e���A��
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;	// �萔�o�b�t�@
	descRange[1].BaseShaderRegister = 1;	// ���W�X�^�ԍ�
	descRange[1].NumDescriptors = 1;		// 1��œǂސ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "t0" �}�e���A���ɂ͂�e�N�X�`��
	// �ʏ�e�N�X�`��, ���Z�e�N�X�`��, ��Z�e�N�X�`��, �g�D�[���e�N�X�`�����ꊇ�œǂ�
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[2].BaseShaderRegister = 0;	// ���W�X�^�ԍ�
	descRange[2].NumDescriptors = 4;		// 1��œǂސ�
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	

	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;		// �����W�̐�
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];	// �Ή����郌���W�ւ̃|�C���^
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;		// �����W�̐�
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];	// �Ή����郌���W�ւ̃|�C���^
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	// ���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// �G���J��Ԃ����(U����)
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// �G���J��Ԃ����(V����)
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;		// �G���J��Ԃ����(W����)
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;		// MIPMAP����Ȃ�
	samplerDesc.MinLOD = 0.0f;					// MIPMAP�����Ȃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// �G�b�W�̂���(������)
	samplerDesc.ShaderRegister = 0;				// �g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// �S���̃f�[�^���V�F�[�_�Ɍ�����
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 2;
	rsd.NumStaticSamplers = 1;
	rsd.pParameters = &rootParam[0];
	rsd.pStaticSamplers = &samplerDesc;

	ID3DBlob* signature = nullptr;		// ���[�g�V�O�l�`�������邽�߂̍ޗ�
	ID3DBlob* error = nullptr;			// �G���[�o�����̑Ώ�
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ���[�g�V�O�l�`���̍쐬
	result = dev->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void Dx12Wrapper::InitPipelineState()
{
	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC layouts[] = { 
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, 
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// �p�C�v���C���X�e�[�g�����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	// ���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layouts;
	gpsDesc.InputLayout.NumElements = _countof(layouts);

	// �V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);		// ���_�V�F�[�_
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);		// �s�N�Z���V�F�[�_

	// �����_�^�[�Q�b�g
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;		// ���̃^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g����
	gpsDesc.NumRenderTargets = 1;							// ��v�����Ă���

	// �[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;		// ���Ƃ�
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;				// �K�{(DSV)
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	// DSV�K�{
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;		// �������ق���Ԃ�

	// ���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �\�ʂ�������Ȃ��āA���ʂ��`�悷��悤�ɂ����

	// ���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;		// ����
	gpsDesc.SampleDesc.Quality = 0;		// ����
	gpsDesc.SampleMask = 0xffffffff;	// �S��1
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`

	auto result = dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&pipelineState));
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

	auto result = dev->CreateCommittedResource(
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

	// ���̃N���A�o�����[���d�v�ȈӖ������̂ō���͍���Ă���
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;	// �[���ő�l��1
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	// �[�x�o�b�t�@�̍쐬
	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&depthBuff));
	

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};	// ���ɐݒ�̕K�v�͂Ȃ�
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
	_dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	_dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;

	// �[�x�o�b�t�@�r���[�̍쐬
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
	// �o���A������
	cmdList->ResourceBarrier(1, &BarrierDesc);
}

void Dx12Wrapper::SetBarrier()
{
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	// �o���A���Z�b�g
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
		;// �҂����Ȃ̂ŉ������Ȃ���
	}
}

std::string Dx12Wrapper::GetExtension(const char* path)
{
	std::string s = path;
	size_t dpoint = s.rfind(".") + 1;		// "."�̏ꏊ��T��
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
	auto wsize = Application::Instance().GetWindowSize();	// ��ʃT�C�Y

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	mappedMatrix.world = world;

	// �J�����̐ݒ�
	auto eyePos = DirectX::XMFLOAT3(0, 20, -15);	// �J�����̈ʒu(���_)
	auto focusPos = DirectX::XMFLOAT3(0, 10, 0);	// �œ_�̈ʒu(�����_)
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// �J�����̏����(�ʏ��(0.0f, 1.0f, 0.0f))	// �J�������Œ肷�邽�߂̂���

	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));	// �J�����s��
													// XMLoadFloat3...XMFloat3��XMVECTOR�ɕϊ�����

	auto aspect = (float)wsize.width / (float)wsize.height;		// �r���[��Ԃ̍����ƕ��̃A�X�y�N�g��
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(3.1415f / 2.0f, aspect, 0.5f, 300.0f);		// �ˉe�s��	// LH...LeftHand�̗�,RH�������
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicLH(30, 30, 0.5f, 300.0f);

	mappedMatrix.viewProj = camera * projection;	// �����鏇�Ԃɂ͋C��t���悤
	mappedMatrix.wvp = world * camera * projection;

	auto lightPos = DirectX::XMFLOAT3(50, 70, -15);
	DirectX::XMMATRIX _lcamera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));
	mappedMatrix.lightVP = _lcamera * lightProj;

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256�A���C�����g�ɍ��킹�Ă���

	auto result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));

	result = constBuff->Map(0, nullptr, (void**)&m);	// �V�F�[�_�ɑ���
	*m = mappedMatrix;

	auto handle = rgstDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;
	dev->CreateConstantBufferView(&cbvDesc, handle);
}

void Dx12Wrapper::InitMaterials()
{
	// �o�b�t�@�̃��T�C�Y
	materialBuffs.resize(pmdLoader->GetMatDatas().size());

	// �q�[�v�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = materialBuffs.size() * 5;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// �q�[�v�쐬
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&matDescriptorHeap));

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256�A���C�����g�ɍ��킹�Ă���

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = size;

	// �n���h���̎擾
	auto handle = matDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// �}�e���A���������
	for (int i = 0; i < materialBuffs.size(); ++i)
	{
		// ���\�[�X�쐬
		result = dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&materialBuffs[i]));

		PMDMaterialData* matMap = nullptr;
		// �}�b�v����
		result = materialBuffs[i]->Map(0, nullptr, (void**)& matMap);
		// ���F
		matMap->diffuseColor = pmdLoader->GetMatDatas()[i].diffuseColor;		// �����F
		matMap->specularColor = pmdLoader->GetMatDatas()[i].specularColor;	// ����F
		matMap->mirrorColor = pmdLoader->GetMatDatas()[i].mirrorColor;		// ���F

		// �A���}�b�v
		materialBuffs[i]->Unmap(0, nullptr);
		cbvDesc.BufferLocation = materialBuffs[i]->GetGPUVirtualAddress();

		auto hptr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// �e�N�X�`�����Ȃ���΁A��������e�N�X�`�����g���B����΁A������g��
		auto image = whiteTexBuff;
		auto spa = blackTexBuff;
		auto sph = whiteTexBuff;
		if (strlen(pmdLoader->GetModelTexturesPath()[i].c_str()) > 0)
		{
			// �Ƃ肠�����ڂ����������牼����
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
			// �����܂ŉ�����

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

		// �g�D�[��������΂�����A�Ȃ���΃f�t�H���g���g��
		auto toon = gradTexBuff;
		if (pmdLoader->GetMatDatas()[i].toonIndex != 0xff)
		{
			toon = toonBuff[pmdLoader->GetMatDatas()[i].toonIndex];
		}

		// �}�e���A���̐F
		dev->CreateConstantBufferView(&cbvDesc, handle);
		handle.ptr += hptr;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// �ʏ�e�N�X�`��
		dev->CreateShaderResourceView(image, &srvDesc, handle);
		handle.ptr += hptr;

		// ���Z(spa)
		dev->CreateShaderResourceView(spa, &srvDesc, handle);
		handle.ptr += hptr;

		// ��Z(sph)
		dev->CreateShaderResourceView(sph, &srvDesc, handle);
		handle.ptr += hptr;

		// �g�D�[��(toon)
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
			// �p�X���Ȃ���Ή摜�͂Ȃ��̂ł��Ȃ��Ă悵
			continue;
		}

		// �X�v���b�^������
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

		// �摜�ǂݍ���&��������
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
	std::fill(data.begin(), data.end(), 0xff);	// ��

	auto result = whiteTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void Dx12Wrapper::CreateBlackTexture()
{
	blackTexBuff = CreateTextureResource(blackTexBuff);

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);	// ��

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

	// �g�D�[���ǂݍ���&��������
	for (int i = 0; i < toon.size(); i++)
	{
		size_t spoint = modelPath.rfind("/");		// "/"�̏ꏊ��T��
		std::string modelToon = modelPath.substr(0, spoint + 1);
		std::string s = GetToonPathFromIndex(modelToon, i);
		auto path = GetWideStringFromString(s);
		
		// �ǂݍ���
		auto result = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_NONE, &metadata[i], toon[i]);

		toonBuff[i] = CreateTextureResource(toonBuff[i], metadata[i].width, metadata[i].height, metadata[i].arraySize);

		// �e�N�X�`����������
		result = toonBuff[i]->WriteToSubresource(
			0,
			nullptr,
			toon[i].GetPixels(),
			metadata[i].width * 4,
			toon[i].GetPixelsSize());

		// �������񂾂�p�ς݂Ȃ̂ŉ��
		toon[i].Release();
	}
}

std::string Dx12Wrapper::GetToonPathFromIndex(const std::string& folder, int idx)
{
	std::string filename = pmdLoader->GetToonTexNames()[idx];
	std::string path = "toon/";
	path += filename;

	// �t�@�C���V�X�e���I�u�W�F�N�g�ւ̃p�X���L�����ǂ����𔻒f����
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
	// �Ăяo��1���(�����񐔂𓾂�)
	auto bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, nullptr, 0);

	// string��wchar_t��, ����ꂽ�����񐔂Ń��T�C�Y���Ă���
	std::wstring wstr;
	wstr.resize(bsize);

	// �Ăяo��2���(�m�ۍς�wstr�ɕϊ���������R�s�[)
	bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
									str.c_str(), -1, &wstr[0], bsize);

	return wstr;
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd)
{
#ifdef _DEBUG
	CreateDebugLayer();
#endif // _DEBUG

	// DXGI�t�@�N�g���̍쐬
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	InitFeatureLevel();
	CreateSwapChain(hwnd);
	CreateRenderTarget();
	
	// �R�}���h�A���P�[�^�ƃR�}���h���X�g�̐���
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));
	// �t�F���X�̍쐬
	result = dev->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&rgstDescriptorHeap));

	imageManager.reset(new ImageManager(dev));

	//modelPath = "model/vocaloid/�����~�N.pmd";
	modelPath = "model/vocaloid/�����~�Nmetal.pmd";
	//modelPath = "model/vocaloid/�������J.pmd";
	//modelPath = "model/yayoi/��悢�w�b�h_�J�W���A���i��x0.96�j����.pmd";
	//modelPath = "model/hibiki/��ߔe��v1.pmd";
	pmdLoader.reset(new PMDLoader(modelPath, dev));

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
	dxgiFactory->Release();
	swapChain->Release();
	dev->Release();
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
	// ���͊֘A
	unsigned char keyState[256] = {};

	Vector3 pos = {};
	Vector3 angle = {};

	if (GetKeyboardState(keyState))
	{
		// ���s�ړ�
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

		// ��]
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
	m->world *= DirectX::XMMatrixRotationY(angle.x);	// ��]
	m->world *= DirectX::XMMatrixRotationX(angle.y);
}

void Dx12Wrapper::Draw()
{
	// ���߂̃N���A
	ClearCmd(pipelineState, rootSignature);

	// �r���[�|�[�g�ƃV�U�[�ݒ�
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissorRect);

	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	bbIdx = swapChain->GetCurrentBackBufferIndex();		// �ޯ��ޯ̧���ޯ���𒲂ׂ�
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };	// �N���A�J���[�ݒ�
	cmdList->OMSetRenderTargets(1, &heapStart, false, &dsvHeap->GetCPUDescriptorHandleForHeapStart());		// �����_�[�^�[�Q�b�g�ݒ�
	cmdList->ClearDepthStencilView(dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);	// �[�x�o�b�t�@�̃N���A

	// �o���A�̉���(�������珑�����݂��n�܂�)
	UnlockBarrier(backBuffers[bbIdx]);

	// ��ʂ̃N���A(������������݂ɓ���)
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// �N���A

	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &pmdLoader->GetVbView());
	cmdList->IASetIndexBuffer(&pmdLoader->GetIbView());


	// ���f���̕`��
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


	// �o���A�̃Z�b�g
	SetBarrier();

	cmdList->Close();	// �N���[�Y
	ExecuteCmd();
	WaitExecute();


	swapChain->Present(1, 0);	// �`��
}
