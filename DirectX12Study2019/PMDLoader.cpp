#include "PMDLoader.h"


PMDLoader::PMDLoader(const std::string& filepath)
{
	// ���f���̓ǂݍ���
	FILE* fp;
	errno_t error;
	error = fopen_s(&fp, filepath.c_str(), "rb");

	// �ǂݍ��߂Ȃ������ꍇ�x����f��
	assert(error == 0);

	// �w�b�_�ǂݍ���
	PMD pmdData;
	fread(&pmdData.magic, sizeof(pmdData.magic), 1, fp);
	fread(&pmdData.version, sizeof(pmdData.version), 1, fp);
	fread(&pmdData.modelName, sizeof(pmdData.modelName), 1, fp);
	fread(&pmdData.comment, sizeof(pmdData.comment), 1, fp);

	// ���_���ǂݍ���
	unsigned int vertexCount;
	fread(&vertexCount, sizeof(vertexCount), 1, fp);
	// ���_�̐��������_���X�g�ǂݍ���
	vertexDatas.resize(vertexCount * (sizeof(PMDVertexData) - 2));
	fread(vertexDatas.data(), vertexDatas.size(), 1, fp);

	// �ʒ��_���ǂݍ���
	unsigned int faceVertexCount;
	fread(&faceVertexCount, sizeof(faceVertexCount), 1, fp);
	// �ʒ��_���X�g�ǂݍ���
	faceVertices.resize(faceVertexCount);
	fread(faceVertices.data(), faceVertices.size() * sizeof(unsigned short), 1, fp);

	// �}�e���A�����ǂݍ���
	unsigned int matCount;
	fread(&matCount, sizeof(matCount), 1, fp);
	// �}�e���A���ǂݍ���
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

	// �����ǂݍ���
	unsigned short boneCount = 0;
	fread(&boneCount, sizeof(boneCount), 1, fp);
	// ���ǂݍ���
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

	// IK���ǂݍ���
	unsigned short ikNum = 0;
	fread(&ikNum, sizeof(ikNum), 1, fp);
	// IK�ǂݍ���(���͏ȗ�)
	for (int i = 0; i < ikNum; ++i)
	{
		fseek(fp, 4, SEEK_CUR);
		unsigned char ikChainNum = 0;
		fread(&ikChainNum, sizeof(ikChainNum), 1, fp);
		fseek(fp, 6, SEEK_CUR);
		fseek(fp, ikChainNum * sizeof(unsigned short), SEEK_CUR);
	}

	// �\��ǂݍ���
	unsigned short skinNum = 0;
	fread(&skinNum, sizeof(skinNum), 1, fp);
	// �\��ǂݍ���(���͏ȗ�)
	for (int i = 0; i < skinNum; ++i)
	{
		fseek(fp, 20, SEEK_CUR);
		unsigned int vertNum = 0;
		fread(&vertNum, sizeof(vertNum), 1, fp);
		fseek(fp, 1, SEEK_CUR);
		fseek(fp, 16 * vertNum, SEEK_CUR);
	}

	// �\���p�\��(���͏ȗ�)
	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	// �\���p�{�[����(���͏ȗ�)
	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	// �\���{�[�����X�g(���͏ȗ�)
	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);
	fseek(fp, 3 * dispBoneNum, SEEK_CUR);

	// �p��
	// �p���Ή��t���O(���͏ȗ�)
	unsigned char englishFlg = 0;
	fread(&englishFlg, sizeof(englishFlg), 1, fp);
	if (englishFlg)
	{
		// ���f����20�o�C�g+256�o�C�g�R�����g
		fseek(fp, 20 + 256, SEEK_CUR);
		// �{�[����20�o�C�g*�{�[����
		fseek(fp, boneCount * 20, SEEK_CUR);
		// (�\�-1)*20�o�C�g�B-1�Ȃ̂̓x�[�X�����Ԃ�
		fseek(fp, (skinNum - 1) * 20, SEEK_CUR);
		// �{�[����*50�o�C�g
		fseek(fp, boneDispNum * 50, SEEK_CUR);
	}

	// �g�D�[�����ǂݍ���
	fread(toonTexNames.data(), sizeof(char) * 100, toonTexNames.size(), fp);

	fclose(fp);
}

PMDLoader::~PMDLoader()
{
}

std::string PMDLoader::GetModelTexturePath(const std::string& modelpath, const char* texpath)
{
	auto spoint = modelpath.rfind("/");		// "/"���t����T�� 
	auto path = (modelpath.substr(0, spoint) + "/" + texpath);		// �߽�̍���
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
