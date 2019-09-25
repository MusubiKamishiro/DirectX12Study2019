#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>

#include <DirectXMath.h>


struct Vector3
{
	float x, y, z;
};

struct Vertex {
	DirectX::XMFLOAT3 pos;		// ���W
};

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
	// ���_�o�b�t�@�̍쐬
	void CreateVertexBuffer(HRESULT& result);

	// �V�F�[�_�̏�����
	void InitShader(HRESULT& result);
	ID3DBlob* vertexShader = nullptr;	// ���_�V�F�[�_
	ID3DBlob* pixelShader = nullptr;	// �s�N�Z���V�F�[�_
	D3D12_VIEWPORT viewport;	// �r���[�|�[�g
	D3D12_RECT scissorRect;		// �V�U�[���N�g
	// ���[�g�V�O�l�`���̏�����
	void InitRootSignatur(HRESULT& result);
	ID3D12RootSignature* rootSignature = nullptr;	// ���ꂪ�ŏI�I�ɗ~�����I�u�W�F�N�g
	// �p�C�v���C���X�e�[�g�̏�����
	void InitPipelineState(HRESULT& result);
	ID3D12PipelineState* pipelineState = nullptr;

	// ���߂̃N���A
	void ClearCmd(ID3D12PipelineState* pipelinestate, ID3D12RootSignature* rootsignature);

	// �o���A�̉���
	void UnlockBarrier(ID3D12Resource* buffer);
	// �o���A�̃Z�b�g
	void SetBarrier();

	// �R�}���h�L���[�ɓ�����
	void ExecuteCmd();
	// �҂�
	void WaitExecute();
	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// �����_�[�^�[�Q�b�g�r���[�p�̃q�[�v
	std::vector<ID3D12Resource*> backBuffers;

	ID3D12Resource* vertexBuffer = nullptr;	// ���_�o�b�t�@
	ID3D12Resource* indexBuffer = nullptr;	// �C���f�b�N�X�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// ���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// �C���f�b�N�X�o�b�t�@�r���[

	UINT64 fenceValue = 0;
	UINT bbIdx = 0;
	
	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// �o���A

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

