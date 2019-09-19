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
	int renderTargetsNum = swapCDesc.BufferCount;
	// �����_�[�^�[�Q�b�g���Ԃ�m��
	renderTargets.resize(renderTargetsNum);
	// �����_�[�^�[�Q�b�g�̍쐬
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescH = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// �e�޽�������̎g�p���黲�ނ��v�Z���Ƃ�
	auto rtvSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (int i = 0; i < renderTargetsNum; ++i)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));	// �L�����o�X���擾
		dev->CreateRenderTargetView(renderTargets[i], nullptr, cpuDescH);	// �L�����o�X�ƐE�l��R�Â���
		cpuDescH.ptr += rtvSize;	// �����_�[�^�[�Q�b�g�r���[�̃T�C�Y�Ԃ񂸂炷
	}
}

void Dx12Wrapper::InitScreen()
{
	// ��ʂ̃N���A
	auto heapStart = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto bbIdx = swapChain->GetCurrentBackBufferIndex();		// �ޯ��ޯ̧���ޯ���𒲂ׂ�
	heapStart.ptr += bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	float clearColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };				// �N���A�J���[�ݒ�
	cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);		// �����_�[�^�[�Q�b�g�ݒ�
	cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);	// �N���A
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
	
	// ����ޱ۹���ƺ����ؽĂ̐���
	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	// ���߂��ĂԑO�Ƀ��Z�b�g
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
