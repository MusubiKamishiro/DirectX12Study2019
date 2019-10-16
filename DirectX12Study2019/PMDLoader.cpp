#include "PMDLoader.h"
#include "d3dx12.h"

#include "Dx12Device.h"

#pragma comment(lib,"d3d12.lib")

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

	CreateView();
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

void PMDLoader::CreateView()
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = vertexDatas.size();	// 頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	auto dev = Dx12Device::Instance().GetDevice();
	// 頂点バッファの作成
	auto result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	// インデックスバッファの作成
	result = dev->CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(faceVertices.size() * sizeof(faceVertices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	D3D12_RANGE range = { 0,0 };
	char* vertexMap = nullptr;
	result = vertexBuffer->Map(0, &range, (void**)& vertexMap);
	std::copy(vertexDatas.begin(), vertexDatas.end(), vertexMap);
	vertexBuffer->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = 38;	// 頂点1つあたりのバイト数
	vbView.SizeInBytes = vertexDatas.size();		// データ全体のサイズ

	unsigned short* ibuffptr = nullptr;
	result = indexBuffer->Map(0, &range, (void**)& ibuffptr);
	std::copy(faceVertices.begin(), faceVertices.end(), ibuffptr);
	indexBuffer->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();		// バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	// フォーマット(shortなのでR16)
	ibView.SizeInBytes = faceVertices.size() * sizeof(faceVertices[0]);	// 総サイズ
}

const std::vector<char>& PMDLoader::GetVertexDatas()const
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

const D3D12_VERTEX_BUFFER_VIEW& PMDLoader::GetVbView() const
{
	return vbView;
}

const D3D12_INDEX_BUFFER_VIEW& PMDLoader::GetIbView() const
{
	return ibView;
}
