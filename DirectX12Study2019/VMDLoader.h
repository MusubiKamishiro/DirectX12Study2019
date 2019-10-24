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


// �t���[���̈ʒu�Ɖ�]���
struct BoneKeyFrames
{
	BoneKeyFrames() : frameNo(0), quaternion(0, 0, 0, 0) {};
	BoneKeyFrames(int f, DirectX::XMFLOAT4 q) : frameNo(f), quaternion(q) {};
	int frameNo;					// �t���[���ԍ�
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

	// �A�j���[�V�����f�[�^�̏�����
	void InitAnimationData();
	// <�{�[����, �t���[���ʒu�Ɖ�]���>
	std::map<std::string, std::vector<BoneKeyFrames>> animationData;

	// �\��f�[�^�̏�����
	void InitSkinData();
	std::map<std::string, std::vector<SkinKeyFrames>> skinData;

	// ���[�V�����̎��Ԃ����߂�
	void SearchMaxFrame();
	int maxFrame;

public:
	VMDLoader(const std::string& filepath);
	~VMDLoader();

	const std::map<std::string, std::vector<BoneKeyFrames>>& GetAnimationData()const;
	const std::map<std::string, std::vector<SkinKeyFrames>>& GetSkinData()const;
	const int GetMaxFrame()const;
};

