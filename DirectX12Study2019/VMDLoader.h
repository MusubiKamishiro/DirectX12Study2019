#pragma once
#include <DirectXMath.h>

#include <vector>
#include <map>
#include <string>


// ���[�V�����f�[�^
struct VMDMotionData
{
	char boneName[15];					// �{�[����
	unsigned int frameNo;				// �t���[���ԍ�(�Ǎ����͌��݂̃t���[���ʒu��0�Ƃ������Έʒu)
	DirectX::XMFLOAT3 location;			// �ʒu(IK�̎��Ɏg�p�\��)
	DirectX::XMFLOAT4 rotation;			// Quaternion	// ��]
	unsigned char interpolation[64];	// [4][4][4]	// �⊮
};

// �\��f�[�^
struct VMDSkinData
{
	char skinName[15];		// �\�
	unsigned int frameNo;	// �t���[���ԍ�
	float weight;			// �\��̐ݒ�l(�\��X���C�_�[�̒l)
};

// �J�����f�[�^
struct VMDCameraData
{
	unsigned int frameNo;		// �t���[���ԍ�
	float length;				// -(����)		// �����炭�J�������S����̋���	�����̒l��0�̏ꍇ��location�����̂܂܃J�����̍��W�ɂȂ�
	DirectX::XMFLOAT3 location;	// �ʒu			// �J�����̒����_
	DirectX::XMFLOAT3 rotation;	// �I�C���[�p	// X���͕��������]���Ă���̂Œ��� // ��]
	char interpolation[24];		// �����炭[6][4](������) // �⊮
	unsigned int viewIngAngle;	// ���E�p
	char perspective;			// 0:on 1:off	// �p�[�X�y�N�e�B�u
};

// �Ɩ��f�[�^
struct VMDLightData
{
	unsigned int frameNo;		// �t���[���ԍ�
	DirectX::XMFLOAT3 color;	// RGB�e�l/256
	DirectX::XMFLOAT3 location;	// X, Y, Z
};

// �Z���t�V���h�E�f�[�^
struct VMDSelfShadowData
{
	unsigned int frameNo;	// �t���[���ԍ�
	char mode;				// 00-02 // ���[�h
	float distance;			// 0.1 - (dist * 0.00001) // ����
};

// IK�f�[�^�p�\����
struct VMDIKData
{
	char ikBoneName[20];		// IK�{�[����
	unsigned char ikEnabled;	// IK�L��	// 0:off, 1:on
};

// �\���EIK�f�[�^�p�\��
struct VMDVisibleIKData
{
	int frameNo = 0;				// �t���[���ԍ�
	unsigned char visible = 0;		// �\��	// 0:off, 1:on
	int ikCount = 0;				// IK��
	std::vector<VMDIKData> ikDatas;	// IK�f�[�^���X�g
};

// �t���[���̈ʒu�Ɖ�]���
struct BoneKeyFrames
{
	BoneKeyFrames() : frameNo(0), pos(0, 0, 0), quaternion(0, 0, 0, 0) {};
	BoneKeyFrames(int f, DirectX::XMFLOAT3 p, DirectX::XMFLOAT4 q) : frameNo(f), pos(p), quaternion(q) {};
	int frameNo;					// �t���[���ԍ�
	DirectX::XMFLOAT3 pos;			// ���W
	DirectX::XMFLOAT4 quaternion;	// ��]���
};

// �t���[���̈ʒu�ƕ\��X���C�_�[�̒l
struct SkinKeyFrames
{
	SkinKeyFrames() : frameNo(0), weight(0.0f) {};
	SkinKeyFrames(int f, float w) : frameNo(f), weight(w) {};
	int frameNo;
	float weight;
};

class VMDLoader
{
private:
	// vmd�t�@�C���̓ǂݍ���
	void Load(const std::string& filepath);
	std::vector<VMDMotionData> motiondatas;
	std::vector<VMDSkinData> skindatas;
	std::vector<VMDCameraData> cameradatas;
	std::vector<VMDLightData> lightdatas;
	std::vector<VMDSelfShadowData> selfshadowdatas;
	std::vector<VMDVisibleIKData> visibleIKDatas;

	// �A�j���[�V�����f�[�^�̏�����
	void InitAnimationData();
	std::map<std::string, std::vector<BoneKeyFrames>> animationData;	// <�{�[����, �t���[���ʒu�Ɖ�]���>

	// �\��f�[�^�̏�����
	void InitSkinData();
	std::map<std::string, std::vector<SkinKeyFrames>> skinData;

	// �J�����f�[�^���t���[�����Ƀ\�[�g
	void SortCameraData();

	// ���[�V�����̎��Ԃ����߂�
	void SearchMaxFrame();
	int maxFrame;

public:
	VMDLoader(const std::string& filepath);
	~VMDLoader();

	const std::map<std::string, std::vector<BoneKeyFrames>>& GetAnimationData()const;
	const std::map<std::string, std::vector<SkinKeyFrames>>& GetSkinData()const;
	const int GetMaxFrame()const;
	const std::vector<VMDCameraData>& GetCameraData()const;
};

