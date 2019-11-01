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

	// ヘッダ部分は不要なので読まない
	fseek(fp, 50, SEEK_SET);

	// モーションデータ数
	unsigned int motionCount = 0;
	fread(&motionCount, sizeof(motionCount), 1, fp);
	// モーションデータ読み込み
	motiondatas.resize(motionCount);
	for (auto& keyframe : motiondatas)
	{
		fread(&keyframe.boneName,		sizeof(keyframe.boneName),		1, fp);
		fread(&keyframe.frameNo,		sizeof(keyframe.frameNo),		1, fp);
		fread(&keyframe.location,		sizeof(keyframe.location),		1, fp);
		fread(&keyframe.rotation,		sizeof(keyframe.rotation),		1, fp);
		fread(&keyframe.interpolation,	sizeof(keyframe.interpolation),	1, fp);
	}

	// 表情データ数読み込み
	unsigned int skinCount = 0;
	fread(&skinCount, sizeof(skinCount), 1, fp);
	// 表情データ読み込み
	skindatas.resize(skinCount);
	for (auto& skindata : skindatas)
	{
		fread(&skindata.skinName,	sizeof(skindata.skinName),	1, fp);
		fread(&skindata.frameNo,	sizeof(skindata.frameNo),	1, fp);
		fread(&skindata.weight,		sizeof(skindata.weight),	1, fp);
	}

	// カメラデータ数読み込み
	unsigned int cameraCount = 0;
	fread(&cameraCount, sizeof(cameraCount), 1, fp);
	// カメラデータ読み込み
	cameradatas.resize(cameraCount);
	for (auto& camdata : cameradatas)
	{
		fread(&camdata.frameNo,			sizeof(camdata.frameNo),		1, fp);
		fread(&camdata.length,			sizeof(camdata.length),			1, fp);
		fread(&camdata.location,		sizeof(camdata.location),		1, fp);
		fread(&camdata.rotation,		sizeof(camdata.rotation),		1, fp);
		fread(&camdata.interpolation,	sizeof(camdata.interpolation),	1, fp);
		fread(&camdata.viewIngAngle,	sizeof(camdata.viewIngAngle),	1, fp);
		fread(&camdata.perspective,		sizeof(camdata.perspective),	1, fp);
	}

	// 照明データ数読み込み
	unsigned int lightCount = 0;
	fread(&lightCount, sizeof(lightCount), 1, fp);
	// 照明データ読み込み
	lightdatas.resize(lightCount);
	for (auto& lightdata : lightdatas)
	{
		fread(&lightdata.frameNo,	sizeof(lightdata.frameNo),	1, fp);
		fread(&lightdata.color,		sizeof(lightdata.color),	1, fp);
		fread(&lightdata.location,	sizeof(lightdata.location),	1, fp);
	}

	// セルフシャドウデータ数読み込み
	unsigned int selfshadowCount = 0;
	fread(&selfshadowCount, sizeof(selfshadowCount), 1, fp);
	// セルフシャドウデータ読み込み
	for (auto& sddata : selfshadowdatas)
	{
		fread(&sddata.frameNo,	sizeof(sddata.frameNo),		1, fp);
		fread(&sddata.mode,		sizeof(sddata.mode),		1, fp);
		fread(&sddata.distance,	sizeof(sddata.distance),	1, fp);
	}
}

void VMDLoader::InitAnimationData()
{
	std::map<std::string, std::vector<BoneKeyFrames>> notSortAnimData;
	// 対象の骨に追加
	for (auto& f : motiondatas)
	{
		notSortAnimData[f.boneName].emplace_back(BoneKeyFrames(f.frameNo, f.location, f.rotation));
	}

	// ソート
	for (auto& animData : notSortAnimData)
	{
		auto bonename = animData.first;		// ボーン名
		auto frameNo = animData.second;		// キーフレーム

		if (frameNo.size() == 1)
		{
			animationData[bonename].emplace_back(BoneKeyFrames(frameNo[0].frameNo, frameNo[0].pos, frameNo[0].quaternion));
			continue;	// アニメーションがないならソートする必要なし
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
			// ソートしたものを追加
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

	// ソート
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
			// ソートしたものを追加
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
