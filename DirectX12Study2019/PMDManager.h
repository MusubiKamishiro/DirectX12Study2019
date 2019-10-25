#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

#include <map>
#include <vector>
#include <array>
#include <string>

#include "VMDLoader.h"

class ImageManager;


// �w�b�_�t�@�C��
struct PMDHeader
{
	char magic[3];		// "Pmd"
	float version;		// �o�[�W����
	char modelName[20];	// ���f���̖��O
	char comment[256];	// ����҃R�����g
};

// ���_�f�[�^
struct PMDVertexData
{
	DirectX::XMFLOAT3 pos;		// x, y, z					// ���W
	DirectX::XMFLOAT3 normalVec;// nx, ny, nz				// �@���x�N�g��
	DirectX::XMFLOAT2 uv;		// u, v						// UV���W			// MMD�͒��_UV
	unsigned short boneNum[2];	// �{�[���ԍ�1�A�ԍ�2		// ���f���ό`(���_�ړ�)���ɉe��
	unsigned char boneWeight;	// �{�[��1�ɗ^����e���x	// min:0 max:100	// �{�[��2�ւ̉e���x�́A(100 - bone_weight)
	unsigned char edgeFlag;		// 0:�ʏ�A1:�G�b�W����		// �G�b�W(�֊s)���L���̏ꍇ
	unsigned char kuuhaku[2];	// �p�e�B���O�p
};

// �}�e���A���f�[�^
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

// �\��p�̒��_�f�[�^
struct PMDSkinVertData
{
	unsigned int skinVertIndex = 0;	// �\��p�̒��_�̔ԍ�(���_���X�g�ɂ���ԍ�)
	DirectX::XMFLOAT3 skinVertPos;	// x, y, z // �\��p�̒��_�̍��W(���_���̂̍��W)
};

// �\��̃f�[�^
struct PMDSkinData
{
	char skinName[20];			// �\�
	unsigned int skinVertCount;	// �\��p�̒��_��
	char skinType;				// �\��̎�� // 0�Fbase�A1�F�܂�A2�F�ځA3�F���b�v�A4�F���̑�
	std::vector<PMDSkinVertData> skinVertData;	// �\��p�̒��_�̃f�[�^(16Bytes/vert)
};

// ���̃f�[�^
struct PMDBoneData
{
	char boneName[20];					// �{�[����
	unsigned short parentBoneIndex;		// �e�{�[���ԍ�(�Ȃ��ꍇ��0xFFFF)
	unsigned short tailPosBoneIndex;	// tail�ʒu�̃{�[���ԍ�(�`�F�[�����[�̏ꍇ��0xFFFF)	// �e�F�q��1�F���Ȃ̂ŁA��Ɉʒu���ߗp
	unsigned char boneType;				// �{�[���̎��
	unsigned short ikParentBoneIndex;	// IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0)
	DirectX::XMFLOAT3 boneHeadPos;		// �{�[���̃w�b�h�̈ʒu		// x, y, z
};

struct BoneNode
{
	int boneIdx = 0;					// �{�[���s��z��ƑΉ�
	DirectX::XMFLOAT3 startPos;			// �{�[���n�_(�֐ߏ������W)
	DirectX::XMFLOAT3 endPos;			// �{�[���I�_(���̊֐ߍ��W)
	std::vector<BoneNode*> children;	// �q�������ւ̃����N
};


///PMD�̓ǂݍ��݂ƕ`����s���N���X
class PMDManager
{
private:
	// PMD�f�[�^�̓ǂݍ���
	void Load(const std::string& filepath);
	std::vector<PMDVertexData> vertexDatas;		// ���_�f�[�^
	std::vector<unsigned short> faceVertices;	// �ʒ��_�f�[�^
	std::vector<PMDMaterialData> matDatas;		// �}�e���A���f�[�^
	std::vector<PMDSkinData> skinDatas;			// �\��f�[�^
	std::vector<PMDBoneData> bones;				// ��
	std::array<char[100], 10> toonTexNames;		// �g�D�[���e�N�X�`���̖��O, �Œ�
	std::vector<std::string> modelTexturesPath;	// ���f���ɒ���t����e�N�X�`���̃p�X(���g���Ȃ��Ƃ�������)

	// �r���[�̍쐬
	void CreateView();
	ID3D12Resource* vertexBuffer = nullptr;	// ���_�o�b�t�@
	ID3D12Resource* indexBuffer = nullptr;	// �C���f�b�N�X�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// ���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// �C���f�b�N�X�o�b�t�@�r���[

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
	ID3D12Resource* gradTexBuff;
	// �g�D�[���e�N�X�`���쐬
	void CreateToonTexture(const std::string& filepath);
	std::vector<ID3D12Resource*> toonBuff;

	// �}�e���A���̏�����
	void InitMaterials();
	ID3D12DescriptorHeap* matDescriptorHeap;	// �}�e���A���p�f�X�N���v�^�q�[�v
	std::vector<ID3D12Resource*> materialBuffs;	// �}�e���A���p�o�b�t�@(�}�e���A��1�ɂ�1��)

	// ���쐬
	void CreateBone();
	DirectX::XMMATRIX* matMap = nullptr;
	ID3D12Resource* boneBuff;		// �{�[���p�o�b�t�@
	ID3D12DescriptorHeap* boneHeap;	// �{�[���p�q�[�v
	std::vector<DirectX::XMMATRIX> boneMatrices;	// �{�[���s��]���p
	std::map<std::string, BoneNode> boneMap;		// �{�[���}�b�v

	// �\��쐬
	void CreateSkin();
	std::map<std::string, std::vector<PMDSkinVertData>> skinMap;

	std::shared_ptr<ImageManager> imageManager;

	// ���f���̃e�N�X�`���̃p�X���l��
	std::string GetModelTexturePath(const std::string& modelpath, const char* texpath);

	// �g���q���擾����
	//@param path �Ώۃp�X�̕�����
	//@return �g���q
	std::string GetExtension(const char* path);

	//�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
	//@param path �Ώۂ̃p�X������
	//@param splitter ��؂蕶��
	//@return �����O��̕�����y�A
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

	// string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	//@param str �}���`�o�C�g������
	//@return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromString(std::string& str);

	// �e�N�X�`�����\�[�X�̍쐬
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
		const unsigned int height = 4, const unsigned int arraySize = 1);

	// �C���f�b�N�X�����Ƀg�D�[���̃p�X�����炤
	std::string GetToonPathFromIndex(const std::string& folder, int idx);

	// ���̉�]���q�܂œ`����
	void RecursiveMatrixMultply(BoneNode& node, DirectX::XMMATRIX& inMat);

	// ������]������
	//@param quaternion
	void RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& quaternion);
	void RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT4& q2, float t = 0.0f);

	void MotionUpdate(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, const int& frame);

	void ChangeSkin(const std::string& skinname);
	void SkinUpdate(const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& frame);

	bool flag = true;
public:
	PMDManager(const std::string& filepath);
	~PMDManager();

	void Update(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& nowframe);
	void Draw(ID3D12GraphicsCommandList* cmdList);
};

