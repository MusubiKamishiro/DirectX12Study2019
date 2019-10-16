#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class ImageManager;
class PMDLoader;


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

struct WVP
{
	DirectX::XMMATRIX world;		// ���[���h
	DirectX::XMMATRIX viewProj;		// �r���[�v���W�F�N�V����
	DirectX::XMMATRIX wvp;			// �����ς�
	DirectX::XMMATRIX lightVP;		// ���C�g�r���[�v���W�F�N�V����
};


class Dx12Wrapper
{
private:
	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;	// ���W�X�^�f�X�N���v�^�q�[�v
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

	// �e�N�X�`�����\�[�X�̍쐬
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
												const unsigned int height = 4, const unsigned int arraySize = 1);

	// �[�x�o�b�t�@�̍쐬
	void CreateDepthBuff();
	ID3D12Resource* depthBuff = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;

	// �萔�o�b�t�@�̏�����
	void InitConstants();
	ID3D12Resource* constBuff = nullptr;	// �萔�o�b�t�@
	WVP* m = nullptr;
	WVP mappedMatrix;

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

	std::shared_ptr<ImageManager> imageManager;

	// �g���q���擾����
	//@param path �Ώۃp�X�̕�����
	//@return �g���q
	std::string GetExtension(const char* path);

	//�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	//@param path �Ώۂ̃p�X������
	//@param splitter ��؂蕶��
	//@return �����O��̕�����y�A
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

	// PMD�֘A
	std::string modelPath;	// ���f���̃p�X
	// PMD��ǂݍ���
	std::shared_ptr<PMDLoader> pmdLoader;
	
	// �}�e���A���̏�����
	void InitMaterials();
	ID3D12DescriptorHeap* matDescriptorHeap;	// PMD�}�e���A���p�f�X�N���v�^�q�[�v
	std::vector<ID3D12Resource*> materialBuffs;	// PMD�}�e���A���p�o�b�t�@(�}�e���A��1�ɂ�1��)
	// ���f���̃e�N�X�`���̍쐬
	void CreateModelTexture();
	std::vector<ID3D12Resource*> modelTexBuff;	// �ʏ�e�N�X�`��
	std::vector<ID3D12Resource*> spaBuff;		// ���Z�e�N�X�`��
	std::vector<ID3D12Resource*> sphBuff;		// ��Z�e�N�X�`��
	// ���e�N�X�`���쐬
	void CreateWhiteTexture();
	ID3D12Resource* whiteTexBuff;
	// ���e�N�X�`���쐬
	void CreateBlackTexture();
	ID3D12Resource* blackTexBuff;
	// �g�D�[�����Ȃ��������Ɏg�p����e�N�X�`���쐬
	void CreateGraduationTextureBuffer();
	ID3D12Resource* gradTexBuff;				// �O���f�[�V�����e�N�X�`��
	// �g�D�[���e�N�X�`���쐬
	void CreateToonTexture();
	std::vector<ID3D12Resource*> toonBuff;		// �g�D�[���e�N�X�`��
	// �C���f�b�N�X�����Ƀg�D�[���̃p�X�����炤
	std::string GetToonPathFromIndex(const std::string& folder, int idx);

	// string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	//@param str �}���`�o�C�g������
	//@return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromString(std::string& str);

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

