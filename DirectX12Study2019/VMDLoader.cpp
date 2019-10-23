#include "VMDLoader.h"


VMDLoader::VMDLoader(const std::string& filepath)
{
	Load(filepath);
	InitAnimationData();
	SearchMaxFrame();
}

VMDLoader::~VMDLoader()
{
}

void VMDLoader::Load(const std::string& filepath)
{
	FILE* fp;
	errno_t error;
	error = fopen_s(&fp, filepath.c_str(), "rb");

	// �w�b�_�����͕s�v�Ȃ̂œǂ܂Ȃ�
	fseek(fp, 50, SEEK_SET);

	// ���[�V�����f�[�^��
	unsigned int motionCount;
	fread(&motionCount, sizeof(motionCount), 1, fp);

	// ���[�V�����f�[�^
	motiondata.resize(motionCount);

	// ���[�V�����f�[�^�ǂݍ���
	for (auto& keyframe : motiondata)
	{
		fread(&keyframe.boneName,		sizeof(keyframe.boneName),		1, fp);
		fread(&keyframe.frameNo,		sizeof(keyframe.frameNo),		1, fp);
		fread(&keyframe.location,		sizeof(keyframe.location),		1, fp);
		fread(&keyframe.rotation,		sizeof(keyframe.rotation),		1, fp);
		fread(&keyframe.interpolation,	sizeof(keyframe.interpolation),	1, fp);
	}
}

void VMDLoader::InitAnimationData()
{
	std::map<std::string, std::vector<KeyFlames>> notSortAnimData;
	// �Ώۂ̍��ɒǉ�
	for (auto& f : motiondata)
	{
		notSortAnimData[f.boneName].emplace_back(KeyFlames(f.frameNo, f.rotation));
	}

	// �\�[�g
	for (auto& animData : notSortAnimData)
	{
		auto bonename = animData.first;		// �{�[����
		auto frameNo = animData.second;		// �L�[�t���[��

		if (frameNo.size() == 1)
		{
			animationData[bonename].emplace_back(KeyFlames(frameNo[0].frameNo, frameNo[0].quaternion));
			continue;	// �A�j���[�V�������Ȃ��Ȃ�\�[�g����K�v�Ȃ�
		}

		for (int i = 0; i < frameNo.size() - 1; i++)
		{
			for (int j = i; j < frameNo.size(); j++)
			{
				if (frameNo[i].frameNo > frameNo[j].frameNo)
				{
					auto f = frameNo[i];

					frameNo[i] = frameNo[j];
					frameNo[j] = f;
				}
			}
		}
		for (int i = 0; i < frameNo.size(); i++)
		{
			// �\�[�g�������̂�ǉ�
			animationData[bonename].emplace_back(KeyFlames(frameNo[i].frameNo, frameNo[i].quaternion));
		}
	}
}

void VMDLoader::SearchMaxFrame()
{
	for (auto& anim : animationData)
	{
		auto frame = anim.second;
		int fcnt = frame[frame.size() - 1].frameNo;
		if (maxFrame < fcnt)
		{
			maxFrame = fcnt;
		}
	}
}

const std::map<std::string, std::vector<KeyFlames>>& VMDLoader::GetAnimationData() const
{
	return animationData;
}

const int VMDLoader::GetMaxFrame() const
{
	return maxFrame;
}
