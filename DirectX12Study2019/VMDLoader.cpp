#include "VMDLoader.h"


VMDLoader::VMDLoader(const std::string& filepath)
{
	Load(filepath);
	InitAnimationData();
	InitSkinData();
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
	unsigned int motionCount = 0;
	fread(&motionCount, sizeof(motionCount), 1, fp);

	// ���[�V�����f�[�^
	motiondatas.resize(motionCount);

	// ���[�V�����f�[�^�ǂݍ���
	for (auto& keyframe : motiondatas)
	{
		fread(&keyframe.boneName,		sizeof(keyframe.boneName),		1, fp);
		fread(&keyframe.frameNo,		sizeof(keyframe.frameNo),		1, fp);
		fread(&keyframe.location,		sizeof(keyframe.location),		1, fp);
		fread(&keyframe.rotation,		sizeof(keyframe.rotation),		1, fp);
		fread(&keyframe.interpolation,	sizeof(keyframe.interpolation),	1, fp);
	}

	// �\��f�[�^���ǂݍ���
	unsigned int skinCount = 0;
	fread(&skinCount, sizeof(skinCount), 1, fp);

	// �\��f�[�^�ǂݍ���
	skindatas.resize(skinCount);
	for (auto& skindata : skindatas)
	{
		fread(&skindata.skinName,	sizeof(skindata.skinName),	1, fp);
		fread(&skindata.frameNo,	sizeof(skindata.frameNo),	1, fp);
		fread(&skindata.weight,		sizeof(skindata.weight),	1, fp);
	}
}

void VMDLoader::InitAnimationData()
{
	std::map<std::string, std::vector<BoneKeyFrames>> notSortAnimData;
	// �Ώۂ̍��ɒǉ�
	for (auto& f : motiondatas)
	{
		notSortAnimData[f.boneName].emplace_back(BoneKeyFrames(f.frameNo, f.location, f.rotation));
	}

	// �\�[�g
	for (auto& animData : notSortAnimData)
	{
		auto bonename = animData.first;		// �{�[����
		auto frameNo = animData.second;		// �L�[�t���[��

		if (frameNo.size() == 1)
		{
			animationData[bonename].emplace_back(BoneKeyFrames(frameNo[0].frameNo, frameNo[0].pos, frameNo[0].quaternion));
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
			animationData[bonename].emplace_back(BoneKeyFrames(frameNo[i].frameNo, frameNo[i].pos, frameNo[i].quaternion));
		}
	}
}

void VMDLoader::InitSkinData()
{
	std::map<std::string, std::vector<SkinKeyFrames>> notSortSkinData;
	for (auto& f : skindatas)
	{
		notSortSkinData[f.skinName].emplace_back(SkinKeyFrames(f.frameNo, f.weight));
	}

	// �\�[�g
	for (auto& skin : notSortSkinData)
	{
		auto skinname = skin.first;
		auto frameNo = skin.second;

		if (frameNo.size() == 1)
		{
			skinData[skinname].emplace_back(SkinKeyFrames(frameNo[0].frameNo, frameNo[0].weight));
			continue;
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
			skinData[skinname].emplace_back(SkinKeyFrames(frameNo[i].frameNo, frameNo[i].weight));
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

const std::map<std::string, std::vector<BoneKeyFrames>>& VMDLoader::GetAnimationData() const
{
	return animationData;
}

const std::map<std::string, std::vector<SkinKeyFrames>>& VMDLoader::GetSkinData() const
{
	return skinData;
}

const int VMDLoader::GetMaxFrame() const
{
	return maxFrame;
}
