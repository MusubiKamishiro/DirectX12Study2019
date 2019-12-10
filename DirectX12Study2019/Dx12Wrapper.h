#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class PMDManager;
class VMDLoader;
class PrimitiveManager;
class Plane;
class ImageManager;

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
	std::shared_ptr<ImageManager> imageManager;

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
	

	// �V�F�[�_�̏�����
	void InitShader();
	ID3DBlob* vertexShader = nullptr;	// ���_�V�F�[�_
	ID3DBlob* pixelShader = nullptr;	// �s�N�Z���V�F�[�_
	D3D12_VIEWPORT viewport;	// �r���[�|�[�g
	D3D12_RECT scissorRect;		// �V�U�[���N�g
	// ���[�g�V�O�l�`���̏�����
	void InitRootSignature();
	ID3D12RootSignature* rootSignature = nullptr;
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
	void UnlockBarrier(ID3D12Resource* buffer, const D3D12_RESOURCE_STATES& before, const D3D12_RESOURCE_STATES& after);
	// �o���A�̃Z�b�g
	void SetBarrier(const D3D12_RESOURCE_STATES& before, const D3D12_RESOURCE_STATES& after);

	// �R�}���h�L���[�ɓ�����
	void ExecuteCmd();
	// �҂�
	void WaitExecute();

	// �e�p�o�b�t�@�̍쐬
	void CreateShadowBuff();
	ID3D12Resource* shadowBuff = nullptr;
	ID3D12DescriptorHeap* shadowDsvHeap;
	ID3D12DescriptorHeap* shadowSrvHeap;
	D3D12_VIEWPORT shadowViewport;	// �e�p�r���[�|�[�g
	D3D12_RECT shadowScissorRect;	// �e�p�V�U�[���N�g
	// �e�p�V�F�[�_�[
	void InitShadowShader();
	// �e�p���[�g�V�O�l�`���̏�����
	void InitShadowRootSignature();
	ID3D12RootSignature* shadowRootSignature;
	// �e�p�p�C�v���C���X�e�[�g�̏�����
	void InitShadowPipelineState();
	ID3D12PipelineState* shadowPipelineState;
	ID3DBlob* shadowVertexShader = nullptr;	// ���_�V�F�[�_
	// ���C�g����̎B�e
	void CreateLightView();

	// 1�p�X�ڂ̍쐬
	void CreateFirstPassBuff();
	ID3D12DescriptorHeap* heapFor1stPassRTV;
	ID3D12DescriptorHeap* heapFor1stPassSRV;
	ID3D12Resource* firstPassBuff;

	// �X�N���[��(�y���|��)�e�N�X�`���쐬
	void CreateScreenTexture();
	ID3D12Resource* screenVertexBuffer = nullptr;	// ���_�o�b�t�@
	ID3D12Resource* screenIndexBuffer = nullptr;	// �C���f�b�N�X�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW svbView = {};			// ���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW sibView = {};			// �C���f�b�N�X�o�b�t�@�r���[

	// �ŏI�`��p�̃��[�g�V�O�l�`���쐬
	void InitLastRootSignature();
	ID3D12RootSignature* lastRootSignature = nullptr;
	// �ŏI�`��p�p�C�v���C���X�e�[�g�̏�����
	void InitLastPipelineState();
	ID3D12PipelineState* lastPipelineState = nullptr;

	// �ŏI�`��p�V�F�[�_�[�̏�����
	void InitLastShader();
	ID3DBlob* lastVertexShader = nullptr;	// ���_�V�F�[�_
	ID3DBlob* lastPixelShader = nullptr;	// �s�N�Z���V�F�[�_

	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// �����_�[�^�[�Q�b�g�r���[�p�̃q�[�v
	std::vector<ID3D12Resource*> backBuffers;

	UINT64 fenceValue = 0;
	UINT bbIdx = 0;
	
	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// �o���A

	// ���[�g�V�O�l�`���̍쐬
	ID3D12RootSignature* CreateRootSignature(ID3D12RootSignature* rootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParam, const D3D12_TEXTURE_ADDRESS_MODE& addressMode);

	// PMD�֘A
	std::vector<std::shared_ptr<PMDManager>> pmdManagers;

	// VMD
	std::vector<std::shared_ptr<VMDLoader>> vmdLoaders;

	// �J����VMD
	std::shared_ptr<VMDLoader> vmdCamera;
	std::string cameraPath;

	unsigned int startTime;
	int frame = 0;
	int maxFrame = 0;

	// ��
	std::shared_ptr<PrimitiveManager> primitiveManager;
	std::shared_ptr<Plane> plane;
	ID3D12Resource* floorImgBuff = nullptr;
	ID3D12DescriptorHeap* floorImgHeap = nullptr;

	// Effekseer
	void EffekseerInit();
	EffekseerRenderer::Renderer* efkRenderer;					// �����_���[
	Effekseer::Manager* efkManager;								// �}�l�[�W���[
	EffekseerRenderer::SingleFrameMemoryPool* efkMemoryPool;	// �������v�[��
	EffekseerRenderer::CommandList* efkCmdList;					// �R�}���h���X�g
	Effekseer::Effect* effect;		// �G�t�F�N�g
	Effekseer::Handle efkHandle;

	// imgui�֌W
	void ImGuiInit(HWND hwnd);
	ID3D12DescriptorHeap* imguiHeap;
	void ImGuiDraw();

	bool motionPlayFlag = false;
	bool motionReversePlayFlag = false;
	
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

