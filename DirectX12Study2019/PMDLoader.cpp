#include "PMDLoader.h"


PMDLoader::PMDLoader(const std::string& filepath)
{
	// モデルの読み込み
	FILE* fp;
	errno_t error;
	error = fopen_s(&fp, filepath.c_str(), "rb");

	// 読み込めなかった場合警告を吐く
	assert(error == 0);

	// ヘッダ読み込み
	PMD pmdData;
	fread(&pmdData.magic, sizeof(pmdData.magic), 1, fp);
	fread(&pmdData.version, sizeof(pmdData.version), 1, fp);
	fread(&pmdData.modelName, sizeof(pmdData.modelName), 1, fp);
	fread(&pmdData.comment, sizeof(pmdData.comment), 1, fp);

	// 頂点数読み込み
	unsigned int vertexCount;
	fread(&vertexCount, sizeof(vertexCount), 1, fp);
	// 頂点の数だけ頂点リスト読み込み
	vertexDatas.resize(vertexCount * (sizeof(PMDVertexData) - 2));
	fread(vertexDatas.data(), vertexDatas.size(), 1, fp);

	// 面頂点数読み込み
	unsigned int faceVertexCount;
	fread(&faceVertexCount, sizeof(faceVertexCount), 1, fp);
	// 面頂点リスト読み込み
	faceVertices.resize(faceVertexCount);
	fread(faceVertices.data(), faceVertices.size() * sizeof(unsigned short), 1, fp);

	// マテリアル数読み込み
	unsigned int matCount;
	fread(&matCount, sizeof(matCount), 1, fp);
	// マテリアル読み込み
	matDatas.resize(matCount);
	modelTexturesPath.resize(matCount);
	for (int i = 0; i < matDatas.size(); ++i)
	{
		fread(&matDatas[i].diffuseColor,	sizeof(matDatas[i].diffuseColor),		1, fp);
		fread(&matDatas[i].alpha,			sizeof(matDatas[i].alpha),				1, fp);
		fread(&matDatas[i].specularity,		sizeof(matDatas[i].specularity),		1, fp);
		fread(&matDatas[i].specularColor,	sizeof(matDatas[i].specularColor),		1, fp);
		fread(&matDatas[i].mirrorColor,		sizeof(matDatas[i].mirrorColor),		1, fp);
		fread(&matDatas[i].toonIndex,		sizeof(matDatas[i].toonIndex),			1, fp);
		fread(&matDatas[i].edgeFlag,		sizeof(matDatas[i].edgeFlag),			1, fp);
		fread(&matDatas[i].faceVertCount,	sizeof(matDatas[i].faceVertCount),		1, fp);
		fread(&matDatas[i].textureFileName,	sizeof(matDatas[i].textureFileName),	1, fp);

		if (std::strlen(matDatas[i].textureFileName) > 0)
		{
			modelTexturesPath[i] = GetModelTexturePath(filepath, matDatas[i].textureFileName);
		}
	}

	// 骨数読み込み
	unsigned short boneCount = 0;
	fread(&boneCount, sizeof(boneCount), 1, fp);
	// 骨読み込み
	bones.resize(boneCount);
	for (auto& bone : bones)
	{
		fread(&bone.boneName,			sizeof(bone.boneName),			1, fp);
		fread(&bone.parentBoneIndex,	sizeof(bone.parentBoneIndex),	1, fp);
		fread(&bone.tailPosBoneIndex,	sizeof(bone.tailPosBoneIndex),	1, fp);
		fread(&bone.boneType,			sizeof(bone.boneType),			1, fp);
		fread(&bone.ikParentBoneIndex,	sizeof(bone.ikParentBoneIndex),	1, fp);
		fread(&bone.boneHeadPos,		sizeof(bone.boneHeadPos),		1, fp);
	}

	// IK数読み込み
	unsigned short ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);
	// IK読み込み(今は省略)
	for (int i = 0; i < ikNum; ++i)
	{
		fseek(fp, 4, SEEK_CUR);
		unsigned char ikChainNum = 0;
		fread(&ikChainNum, sizeof(ikChainNum), 1, fp);
		fseek(fp, 6, SEEK_CUR);
		fseek(fp, ikChainNum * sizeof(unsigned short), SEEK_CUR);
	}

	// 表情数読み込み
	unsigned short skinNum = 0;
	fread(&skinNum, sizeof(skinNum), 1, fp);
	// 表情読み込み(今は省略)
	for (int i = 0; i < skinNum; ++i)
	{
		fseek(fp, 20, SEEK_CUR);
		unsigned int vertNum = 0;
		fread(&vertNum, sizeof(vertNum), 1, fp);
		fseek(fp, 1, SEEK_CUR);
		fseek(fp, 16 * vertNum, SEEK_CUR);
	}

	// 表示用表情(今は省略)
	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	// 表示用ボーン名(今は省略)
	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	// 表示ボーンリスト(今は省略)
	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);
	fseek(fp, 3 * dispBoneNum, SEEK_CUR);

	// 英名
	// 英名対応フラグ(今は省略)
	unsigned char englishFlg = 0;
	fread(&englishFlg, sizeof(englishFlg), 1, fp);
	if (englishFlg)
	{
		// モデル名20バイト+256バイトコメント
		fseek(fp, 20 + 256, SEEK_CUR);
		// ボーン名20バイト*ボーン数
		fseek(fp, boneCount * 20, SEEK_CUR);
		// (表情数-1)*20バイト。-1なのはベース部分ぶん
		fseek(fp, (skinNum - 1) * 20, SEEK_CUR);
		// ボーン数*50バイト
		fseek(fp, boneDispNum * 50, SEEK_CUR);
	}

	// トゥーン名読み込み
	fread(toonTexNames.data(), sizeof(char) * 100, toonTexNames.size(), fp);

	fclose(fp);
}

PMDLoader::~PMDLoader()
{
}

std::string PMDLoader::GetModelTexturePath(const std::string& modelpath, const char* texpath)
{
	auto spoint = modelpath.rfind("/");		// "/"を逆から探索 
	auto path = (modelpath.substr(0, spoint) + "/" + texpath);		// ﾊﾟｽの合成
	return path;
}

const std::vector<char>& PMDLoader::GetVertexDatas()
{
	return vertexDatas;
}

const std::vector<unsigned short>& PMDLoader::GetFaceVertices()
{
	return faceVertices;
}

const std::vector<PMDMaterialData>& PMDLoader::GetMatDatas()
{
	return matDatas;
}

const std::vector<PMDBoneData>& PMDLoader::GetBones()
{
	return bones;
}

const std::array<char[100], 10>& PMDLoader::GetToonTexNames()
{
	return toonTexNames;
}

const std::vector<std::string>& PMDLoader::GetModelTexturesPath()
{
	return modelTexturesPath;
}
