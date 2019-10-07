#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>
#include <string>

// PMD�̃w�b�_�t�@�C��
struct PMD
{
	char magic[3];		// "Pmd"
	float version;		// �o�[�W����
	char modelName[20];	// ���f���̖��O
	char comment[256];	// ����҃R�����g
};

// ���_�f�[�^
struct PMDVertexData
{
	float pos[3];				// x, y, z					// ���W
	float normalVec[3];			// nx, ny, nz				// �@���x�N�g��
	float uv[2];				// u, v						// UV���W			// MMD�͒��_UV
	unsigned short boneNum[2];	// �{�[���ԍ�1�A�ԍ�2		// ���f���ό`(���_�ړ�)���ɉe��
	unsigned char boneWeight;	// �{�[��1�ɗ^����e���x	// min:0 max:100	// �{�[��2�ւ̉e���x�́A(100 - bone_weight)
	unsigned char edgeFlag;		// 0:�ʏ�A1:�G�b�W����		// �G�b�W(�֊s)���L���̏ꍇ
};

struct PMDMaterialData
{
	DirectX::XMFLOAT3 diffuseColor;	// dr, dg, db	// �����F
	float alpha;					// �����F�̕s�����x
	float specularity;				// �X�y�L������Z(�X�y�L�����̉s��)
	DirectX::XMFLOAT3 specularColor;// sr, sg, sb	// ����F
	DirectX::XMFLOAT3 mirrorColor;	// mr, mg, mb	// ���F(ambient)
	unsigned char toonIndex;		// toon??.bmp	// �g�D�[��
	unsigned char edgeFlag;			// �֊s,�e
	// �p�f�B���O2���\�z�����...�����܂ł�46�o�C�g
	unsigned int faceVertCount;		// �ʒ��_��
	char textureFileName[20];		// �e�N�X�`���t�@�C����
};

struct Vector3
{
	float x, y, z;
};

struct Vertex {
	DirectX::XMFLOAT3 pos;	// ���W
	DirectX::XMFLOAT2 uv;	// UV���W
};

struct WVP {
	DirectX::XMMATRIX world;		// ܰ���
	DirectX::XMMATRIX viewProj;		// �ޭ���ۼު����
	DirectX::XMMATRIX wvp;			// �����ς�
	DirectX::XMMATRIX lightVP;		// ײ��ޭ���ۼު����
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
	void CreateDebugLayer();
	// �t�B�[�`���[���x���̑I��
	void InitFeatureLevel();
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

	void InitConstants();	// �萔�o�b�t�@�̏�����
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

	///��������ꎞ�I�Ȏ����̂��߂̕�
	///�m�F���I��莟��폜���邱��
	ID3D12Resource* texBuff = nullptr;
	ID3D12DescriptorHeap* texHeap = nullptr;
	// �e�N�X�`���摜�̍쐬
	//@param texpath �e�N�X�`���̃p�X
	void CreateTex(std::string& texpath);

	// PMD�֘A
	//@param	���f���̃p�X
	void Pmd(std::string& filepath);
	std::vector<char> pmdVertexDatas;	// PMD���_�f�[�^
	std::vector<unsigned short> pmdFaceVertices;	// PMD�ʒ��_�f�[�^
	std::vector<PMDMaterialData> pmdMatDatas;	// PMD�}�e���A���f�[�^
	// ���_�o�b�t�@�̍쐬
	void CreatePmdVertexBuffer();
	ID3D12Resource* pmdVertexBuffer = nullptr;	// PMD�p���_�o�b�t�@
	ID3D12Resource* pmdIndexBuffer = nullptr;	// PMD�p�C���f�b�N�X�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW pmdVbView = {};	// PMD�p���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW pmdIbView = {};		// PMD�p�C���f�b�N�X�o�b�t�@�r���[
	// �}�e���A���̏�����
	void InitMaterials();
	ID3D12DescriptorHeap* matDescriptorHeap;	// PMD�}�e���A���p�f�X�N���v�^�q�[�v
	std::vector<ID3D12Resource*> materialBuffs;	// PMD�}�e���A���p�o�b�t�@(�}�e���A��1�ɂ�1��)
	// ���f���̃e�N�X�`���̍쐬
	void CreateModelTexture();
	std::vector<ID3D12Resource*> modelTexBuff;
	// ���f���̃e�N�X�`���̃p�X���l��
	std::string GetModelTexturePath(const std::string& modelpath, const char* texpath);
	std::vector<std::string> modelTexturesPath;	// ���f���ɒ���t����e�N�X�`���̃p�X(���g���Ȃ��Ƃ�������)
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

