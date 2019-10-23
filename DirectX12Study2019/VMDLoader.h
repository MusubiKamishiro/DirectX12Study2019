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

// �t���[���̈ʒu�Ɖ�]���
struct KeyFlames
{
	KeyFlames() : frameNo(0), quaternion(0, 0, 0, 0) {};
	KeyFlames(int f, DirectX::XMFLOAT4 q) : frameNo(f), quaternion(q) {};
	int frameNo;					// �t���[���ԍ�
	DirectX::XMFLOAT4 quaternion;	// ��]���
};


class VMDLoader
{
private:
	// vmd�t�@�C���̓ǂݍ���
	void Load(const std::string& filepath);
	std::vector<VMDMotionData> motiondata;

	// �A�j���[�V�����f�[�^�̏�����
	void InitAnimationData();
	// <�{�[����, �t���[���ʒu�Ɖ�]���>
	std::map<std::string, std::vector<KeyFlames>> animationData;

	// ���[�V�����̎��Ԃ����߂�
	void SearchMaxFrame();
	int maxFrame;

public:
	VMDLoader(const std::string& filepath);
	~VMDLoader();

	const std::map<std::string, std::vector<KeyFlames>>& GetAnimationData()const;
	const int GetMaxFrame()const;
};

