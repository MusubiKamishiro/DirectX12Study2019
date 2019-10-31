#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class PMDManager;
class VMDLoader;


struct Vector3
{
	float x, y, z;
};

struct Vertex
{
	Vertex() {};
	Vertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT2 _uv) : pos(_pos), uv(_uv) {};

	DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0, 0, 0);	// ���W
	DirectX::XMFLOAT2 uv = DirectX::XMFLOAT2(0, 0);		// UV���W
};


class Dx12Wrapper
{
private:
	IDXGISwapChain4* swapChain = nullptr;	// �X���b�v�`�F�C��
	// �R�}���h�n
	ID3D12CommandAllocator* cmdAllocator = nullptr;	// �R�}���h�A���P�[�^
	// �R�}���h���X�g�{��
	ID3D12GraphicsCommandList* cmdList = nullptr;	// �R�}���h���X�g
	// �R�}���h���s�̒P��
	ID3D12CommandQueue* cmdQueue = nullptr;			// �R�}���h�L���[
	// �҂��̂��߂̃t�F���X
	ID3D12Fence* fence = nullptr;

	// �f�o�b�O���C���̍쐬
	void CreateDebugLayer();
	// �X���b�v�`�F�C���̍쐬
	void CreateSwapChain(HWND hwnd);
	// �����_�[�^�[�Q�b�g�̍쐬
	void CreateRenderTarget();
	// ���_�o�b�t�@�̍쐬
	void CreateVertexBuffer();

	// �V�F�[�_�̏�����
	void InitShader();
	ID3DBlob* vertexShader = nullptr;	// ���_�V�F�[�_
	ID3DBlob* pixelShader = nullptr;	// �s�N�Z���V�F�[�_
	D3D12_VIEWPORT viewport;	// �r���[�|�[�g
	D3D12_RECT scissorRect;		// �V�U�[���N�g
	// ���[�g�V�O�l�`���̏�����
	void InitRootSignatur();
	ID3D12RootSignature* rootSignature = nullptr;	// ���ꂪ�ŏI�I�ɗ~�����I�u�W�F�N�g
	// �p�C�v���C���X�e�[�g�̏�����
	void InitPipelineState();
	ID3D12PipelineState* pipelineState = nullptr;

	// �[�x�o�b�t�@�̍쐬
	void CreateDepthBuff();
	ID3D12Resource* depthBuff = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;

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

	// PMD�֘A
	std::string modelPath;	// ���f���̃p�X
	std::shared_ptr<PMDManager> pmdManager;

	// VMD
	std::shared_ptr<VMDLoader> vmdLoader;
	std::string vmdPath;	// vmd�̃p�X

	unsigned int startTime;
	unsigned int frame = 0;
	
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

