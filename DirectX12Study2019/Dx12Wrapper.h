#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>


class Dx12Wrapper
{
private:
	// ��{�I�ȓz(DXGI)
	IDXGIFactory6* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;	// �X���b�v�`�F�C��
	// ��{�I�ȓz(�f�o�C�X)
	ID3D12Device* dev = nullptr;
	// �R�}���h�n
	ID3D12CommandAllocator* cmdAllocator = nullptr;	// �R�}���h�A���P�[�^
	// �R�}���h���X�g�{��
	ID3D12GraphicsCommandList* cmdList = nullptr;	// �R�}���h���X�g
	// �R�}���h���s�̒P��
	ID3D12CommandQueue* cmdQueue = nullptr;			// �R�}���h�L���[
	// �҂��̂��߂̃t�F���X
	ID3D12Fence* fence = nullptr;

	// �f�o�b�O���C���̍쐬
	void CreateDebugLayer(HRESULT& result);
	// �t�B�[�`���[���x���̑I��
	void InitFeatureLevel(HRESULT& result);
	// �X���b�v�`�F�C���̍쐬
	void CreateSwapChain(HRESULT& result, HWND hwnd);
	// �����_�[�^�[�Q�b�g�̍쐬
	void CreateRenderTarget(HRESULT& result);

	// ��ʂ̏�����
	void InitScreen();
	// �R�}���h�L���[�ɓ�����
	void ExecuteCmd();
	// �҂�
	void WaitExecute();
	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// �����_�[�^�[�Q�b�g�r���[�p�̃q�[�v
	std::vector<ID3D12Resource*> backBuffers;

	UINT64 fenceValue = 0;

	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// �o���A


public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
};

