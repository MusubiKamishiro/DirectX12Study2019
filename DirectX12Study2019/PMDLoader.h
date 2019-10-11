#pragma once
#include <DirectXMath.h>

#include <vector>
#include <array>
//#include <map>
#include <string>

// PMD�̃w�b�_�t�@�C��
struct PMD
{
	char magic[3];		// "Pmd"
	float version;		// �o�[�W����
	char modelName[20];	// ���f���̖��O
	char comment[256];	// ����҃R�����g
};

// PMD�̒��_�f�[�^
struct PMDVertexData
{
	float pos[3];				// x, y, z					// ���W
	float normalVec[3];			// nx, ny, nz				// �@���x�N�g��
	float uv[2];				// u, v						// UV���W			// MMD�͒��_UV
	unsigned short boneNum[2];	// �{�[���ԍ�1�A�ԍ�2		// ���f���ό`(���_�ړ�)���ɉe��
	unsigned char boneWeight;	// �{�[��1�ɗ^����e���x	// min:0 max:100	// �{�[��2�ւ̉e���x�́A(100 - bone_weight)
	unsigned char edgeFlag;		// 0:�ʏ�A1:�G�b�W����		// �G�b�W(�֊s)���L���̏ꍇ
};

// PMD�̃}�e���A���f�[�^
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

// PMD�̍��̃f�[�^
struct PMDBoneData
{
	char boneName[20];					// �{�[����
	unsigned short parentBoneIndex;		// �e�{�[���ԍ�(�Ȃ��ꍇ��0xFFFF)
	unsigned short tailPosBoneIndex;	// tail�ʒu�̃{�[���ԍ�(�`�F�[�����[�̏ꍇ��0xFFFF)	// �e�F�q��1�F���Ȃ̂ŁA��Ɉʒu���ߗp
	unsigned char boneType;				// �{�[���̎��
	unsigned short ikParentBoneIndex;	// IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0)
	float boneHeadPos[3];				// �{�[���̃w�b�h�̈ʒu		// x, y, z
};


///PMD�̓ǂݍ��݂��s���N���X
class PMDLoader
{
private:
	std::vector<char> vertexDatas;				// ���_�f�[�^
	std::vector<unsigned short> faceVertices;	// �ʒ��_�f�[�^
	std::vector<PMDMaterialData> matDatas;		// �}�e���A���f�[�^
	std::vector<PMDBoneData> bones;				// ��
	std::array<char[100], 10> toonTexNames;		// �g�D�[���e�N�X�`���̖��O, �Œ�
	std::vector<std::string> modelTexturesPath;	// ���f���ɒ���t����e�N�X�`���̃p�X(���g���Ȃ��Ƃ�������)

	// ���f���̃e�N�X�`���̃p�X���l��
	std::string GetModelTexturePath(const std::string& modelpath, const char* texpath);

public:
	PMDLoader(const std::string& filepath);
	~PMDLoader();

	const std::vector<char>& GetVertexDatas();			// PMD���_�f�[�^
	const std::vector<unsigned short>& GetFaceVertices();// PMD�ʒ��_�f�[�^
	const std::vector<PMDMaterialData>& GetMatDatas();	// PMD�}�e���A���f�[�^
	const std::vector<PMDBoneData>& GetBones();			// PMD�̍�
	const std::array<char[100], 10>& GetToonTexNames();		// �g�D�[���e�N�X�`���̖��O, �Œ�
	const std::vector<std::string>& GetModelTexturesPath();	// ���f���ɒ���t����e�N�X�`���̃p�X(���g���Ȃ��Ƃ�������)
};

