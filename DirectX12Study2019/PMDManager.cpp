#include "PMDManager.h"
#include "d3dx12.h"
#include "shlwapi.h"
#include <algorithm>

#include "Dx12Device.h"
#include "Dx12Constants.h"
#include "ImageManager.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib, "shlwapi.lib")


PMDManager::PMDManager(const std::string& filepath)
{
	imageManager.reset(new ImageManager());

	Load(filepath);

	CreateView();
	CreateModelTexture();
	CreateWhiteTexture();
	CreateBlackTexture();
	CreateToonTexture(filepath);
	CreateGraduationTextureBuffer();
	InitMaterials();

	CreateBone();
	CreateSkin();
}

PMDManager::~PMDManager()
{
	vertexBuffer->Release();
	indexBuffer->Release();

	whiteTexBuff->Release();
	blackTexBuff->Release();
	gradTexBuff->Release();

	matDescriptorHeap->Release();
	for(auto& mat : materialBuffs)
	{
		mat->Release();
	}
	boneBuff->Release();
	boneHeap->Release();
}

void PMDManager::Load(const std::string& filepath)
{
	// モデルの読み込み
	FILE* fp;
	errno_t error;
	error = fopen_s(&fp, filepath.c_str(), "rb");

	// 読み込めなかった場合警告を吐く
	assert(error == 0);

	// ヘッダ読み込み
	PMDHeader pmdData;
	fread(&pmdData.magic,		sizeof(pmdData.magic),		1, fp);
	fread(&pmdData.version,		sizeof(pmdData.version),	1, fp);
	fread(&pmdData.modelName,	sizeof(pmdData.modelName),	1, fp);
	fread(&pmdData.comment,		sizeof(pmdData.comment),	1, fp);

	// 頂点数読み込み
	unsigned int vertexCount;
	fread(&vertexCount, sizeof(vertexCount), 1, fp);
	// 頂点の数だけ頂点リスト読み込み
	vertexDatas.resize(vertexCount);
	for (int i = 0; i < vertexDatas.size(); ++i)
	{
		fread(&vertexDatas[i].pos,			sizeof(vertexDatas[i].pos), 1, fp);
		fread(&vertexDatas[i].normalVec,	sizeof(vertexDatas[i].normalVec), 1, fp);
		fread(&vertexDatas[i].uv,			sizeof(vertexDatas[i].uv), 1, fp);
		fread(&vertexDatas[i].boneNum,		sizeof(vertexDatas[i].boneNum), 1, fp);
		fread(&vertexDatas[i].boneWeight,	sizeof(vertexDatas[i].boneWeight), 1, fp);
		fread(&vertexDatas[i].edgeFlag,		sizeof(vertexDatas[i].edgeFlag), 1, fp);
	}

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
	// IK読み込み
	ikDatas.resize(ikNum);
	for (auto& ikData : ikDatas)
	{
		fread(&ikData.ikBoneIndex,			sizeof(ikData.ikBoneIndex),			1, fp);
		fread(&ikData.ikTargetBoneIndex,	sizeof(ikData.ikTargetBoneIndex),	1, fp);
		fread(&ikData.ikChainLength,		sizeof(ikData.ikChainLength),		1, fp);
		fread(&ikData.iterations,			sizeof(ikData.iterations),			1, fp);
		fread(&ikData.controlWeight,		sizeof(ikData.controlWeight),		1, fp);
		ikData.ikChildBoneIndex.resize(ikData.ikChainLength);
		for (auto& child : ikData.ikChildBoneIndex)
		{
			fread(&child, sizeof(child), 1, fp);
		}
	}
	
	// 表情数読み込み
	unsigned short skinNum = 0;
	fread(&skinNum, sizeof(skinNum), 1, fp);
	// 表情読み込み
	skinDatas.resize(skinNum);
	for (auto& skin : skinDatas)
	{
		fread(&skin.skinName,		sizeof(skin.skinName),		1, fp);
		fread(&skin.skinVertCount,	sizeof(skin.skinVertCount),	1, fp);
		fread(&skin.skinType,		sizeof(skin.skinType),		1, fp);

		skin.skinVertData.resize(skin.skinVertCount);
		for (auto& vertdata : skin.skinVertData)
		{
			fread(&vertdata.skinVertIndex,	sizeof(vertdata.skinVertIndex),	1, fp);
			fread(&vertdata.skinVertPos,	sizeof(vertdata.skinVertPos),	1, fp);
		}
	}

	// 表示用表情(省略)
	unsigned char skinDispNum = 0;
	fread(&skinDispNum, sizeof(skinDispNum), 1, fp);
	fseek(fp, skinDispNum * sizeof(unsigned short), SEEK_CUR);

	// 表示用ボーン名(省略)
	unsigned char boneDispNum = 0;
	fread(&boneDispNum, sizeof(boneDispNum), 1, fp);
	fseek(fp, 50 * boneDispNum, SEEK_CUR);

	// 表示ボーンリスト(省略)
	unsigned int dispBoneNum = 0;
	fread(&dispBoneNum, sizeof(dispBoneNum), 1, fp);
	fseek(fp, 3 * dispBoneNum, SEEK_CUR);

	// 英名
	// 英名対応フラグ(省略)
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

void PMDManager::CreateView()
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = vertexDatas.size() * (sizeof(PMDVertexData));	// 頂点情報が入るだけのサイズ
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
	PMDVertexData* vertexMap = nullptr;
	result = vertexBuffer->Map(0, &range, (void**)& vertexMap);
	std::copy(vertexDatas.begin(), vertexDatas.end(), vertexMap);
	vertexBuffer->Unmap(0, nullptr);
	
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(PMDVertexData);	// 頂点1つあたりのバイト数
	vbView.SizeInBytes = vertexDatas.size() * sizeof(PMDVertexData);	// データ全体のサイズ

	unsigned short* ibuffptr = nullptr;
	result = indexBuffer->Map(0, &range, (void**)& ibuffptr);
	std::copy(faceVertices.begin(), faceVertices.end(), ibuffptr);
	indexBuffer->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();		// バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	// フォーマット(shortなのでR16)
	ibView.SizeInBytes = faceVertices.size() * sizeof(faceVertices[0]);	// 総サイズ
}

void PMDManager::CreateModelTexture()
{
	unsigned int matNum = matDatas.size();

	modelTexBuff.resize(matNum);
	spaBuff.resize(matNum);
	sphBuff.resize(matNum);

	for (unsigned int i = 0; i < matNum; ++i)
	{
		if (std::strlen(modelTexturesPath[i].c_str()) == 0)
		{
			// パスがなければ画像はないのでやらなくてよし
			continue;
		}

		// スプリッタがある
		std::string texFileName = modelTexturesPath[i];
		if (count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first.c_str()) == "sph" || GetExtension(namepair.first.c_str()) == "spa")
			{
				texFileName = namepair.second;
			}
			else
			{
				texFileName = namepair.first;
			}
		}

		auto ext = GetExtension(texFileName.c_str());

		// 画像読み込み&書き込み
		if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "tga")
		{
			modelTexBuff[i] = imageManager->Load(texFileName);
		}
		else if (ext == "spa")
		{
			spaBuff[i] = imageManager->Load(texFileName);
		}
		else if (ext == "sph")
		{
			sphBuff[i] = imageManager->Load(texFileName);
		}
	}
}

void PMDManager::CreateWhiteTexture()
{
	whiteTexBuff = imageManager->CreateTextureResource(whiteTexBuff);

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);	// 白

	auto result = whiteTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void PMDManager::CreateBlackTexture()
{
	blackTexBuff = imageManager->CreateTextureResource(blackTexBuff);

	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0x00);	// 黒

	auto result = blackTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * 4, 4 * 4 * 4);
}

void PMDManager::CreateGraduationTextureBuffer()
{
	gradTexBuff = imageManager->CreateTextureResource(gradTexBuff);

	struct Color
	{
		Color() : r(0), g(0), b(0), a(0) {}
		Color(unsigned char inr, unsigned char ing, unsigned char inb, unsigned char ina)
			: r(inr), g(ing), b(inb), a(ina) {}

		unsigned char r, g, b, a;
	};

	std::vector<Color> data;
	data.resize(4 * 256);
	unsigned char brightness = 255;
	for (auto it = data.begin(); it != data.end(); it += 4)
	{
		std::fill_n(it, 4, Color(brightness, brightness, brightness, 0xff));
		--brightness;
	}

	auto result = gradTexBuff->WriteToSubresource(0, nullptr, data.data(), 4 * sizeof(Color), data.size() * sizeof(Color));
}

void PMDManager::CreateToonTexture(const std::string& filepath)
{
	toonBuff.resize(10);

	// トゥーン読み込み&書き込み
	for (int i = 0; i < toonBuff.size(); ++i)
	{
		size_t spoint = filepath.rfind("/");		// "/"の場所を探る
		std::string modelToon = filepath.substr(0, spoint + 1);
		std::string path = GetToonPathFromIndex(modelToon, i);

		toonBuff[i] = imageManager->Load(path);
	}
}

void PMDManager::InitMaterials()
{
	// バッファのリサイズ
	materialBuffs.resize(matDatas.size());

	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = materialBuffs.size() * 5;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto dev = Dx12Device::Instance().GetDevice();
	// ヒープ作成
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&matDescriptorHeap));

	size_t size = sizeof(WVP);
	size = (size + 0xff) & ~0xff;		// 256アライメントに合わせている

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = size;

	// ハンドルの取得
	auto handle = matDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// マテリアル数分作る
	for (int i = 0; i < materialBuffs.size(); ++i)
	{
		// リソース作成
		result = dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&materialBuffs[i]));

		PMDMaterialData* matMap = nullptr;
		// マップする
		result = materialBuffs[i]->Map(0, nullptr, (void**)& matMap);
		// 着色
		matMap->diffuseColor = matDatas[i].diffuseColor;	// 減光色
		matMap->specularColor = matDatas[i].specularColor;	// 光沢色
		matMap->mirrorColor = matDatas[i].mirrorColor;		// 環境色

		// アンマップ
		materialBuffs[i]->Unmap(0, nullptr);
		cbvDesc.BufferLocation = materialBuffs[i]->GetGPUVirtualAddress();

		auto hptr = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// テクスチャがなければ、作った白テクスチャを使う。あれば、それを使う
		auto image = whiteTexBuff;
		auto spa = blackTexBuff;
		auto sph = whiteTexBuff;
		if (strlen(modelTexturesPath[i].c_str()) > 0)
		{
			// とりあえず目が見たいから仮実装
			std::string texFileName = modelTexturesPath[i];
			if (count(texFileName.begin(), texFileName.end(), '*') > 0)
			{
				auto namepair = SplitFileName(texFileName);
				if (GetExtension(namepair.first.c_str()) == "sph" || GetExtension(namepair.first.c_str()) == "spa")
				{
					texFileName = namepair.second;
				}
				else
				{
					texFileName = namepair.first;
				}
			}
			auto ext = GetExtension(texFileName.c_str());
			// ここまで仮実装

			if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "tga")
			{
				image = modelTexBuff[i];
			}
			else if (ext == "spa")
			{
				spa = spaBuff[i];
			}
			else if (ext == "sph")
			{
				sph = sphBuff[i];
			}
		}

		// トゥーンがあればそれを、なければデフォルトを使う
		auto toon = gradTexBuff;
		if (matDatas[i].toonIndex != 0xff)
		{
			toon = toonBuff[matDatas[i].toonIndex];
		}

		// マテリアルの色
		dev->CreateConstantBufferView(&cbvDesc, handle);
		handle.ptr += hptr;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// 通常テクスチャ
		dev->CreateShaderResourceView(image, &srvDesc, handle);
		handle.ptr += hptr;

		// 加算(spa)
		dev->CreateShaderResourceView(spa, &srvDesc, handle);
		handle.ptr += hptr;

		// 乗算(sph)
		dev->CreateShaderResourceView(sph, &srvDesc, handle);
		handle.ptr += hptr;

		// トゥーン(toon)
		dev->CreateShaderResourceView(toon, &srvDesc, handle);
		handle.ptr += hptr;
	}
}

void PMDManager::CreateBone()
{
	boneMatrices.resize(bones.size());
	std::fill(boneMatrices.begin(), boneMatrices.end(), DirectX::XMMatrixIdentity());

	// マップ情報を構築
	for (int idx = 0; idx < bones.size(); ++idx)
	{
		auto& b = bones[idx];
		auto& bNode = boneMap[b.boneName];
		bNode.boneIdx = idx;
		bNode.startPos = b.boneHeadPos;
		bNode.endPos = bones[b.tailPosBoneIndex].boneHeadPos;
	}
	for (auto& b : boneMap)
	{
		if (bones[b.second.boneIdx].parentBoneIndex >= bones.size())
		{
			// 子供がいないならやり直し
			continue;
		}
		auto parentName = bones[bones[b.second.boneIdx].parentBoneIndex].boneName;
		boneMap[parentName].children.push_back(&b.second);		// 親の後ろに子供を追加する
	}

	// ヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto dev = Dx12Device::Instance().GetDevice();
	// ヒープ作成
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&boneHeap));

	size_t size = sizeof(DirectX::XMMATRIX) * boneMatrices.size();
	size = (size + 0xff) & ~0xff;		// 256アライメントに合わせている

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&boneBuff));

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = boneBuff->GetGPUVirtualAddress();
	desc.SizeInBytes = size;

	auto handle = boneHeap->GetCPUDescriptorHandleForHeapStart();
	dev->CreateConstantBufferView(&desc, handle);

	result = boneBuff->Map(0, nullptr, (void**)& matMap);
	std::copy(boneMatrices.begin(), boneMatrices.end(), matMap);

}

void PMDManager::CreateSkin()
{
	// baseの表情
	auto& skin = skinDatas[0];
	auto& skinBase = skinMap[skin.skinName];
	skinBase.resize(skin.skinVertCount);
	for (int i = 0; i < skinBase.size(); ++i)
	{
		skinBase[i].skinVertIndex = skin.skinVertData[i].skinVertIndex;
		skinBase[i].skinVertPos = skin.skinVertData[i].skinVertPos;
	}

	// base以外の情報
	// この時点で頂点インデックスを全体のものに合わせる
	for (int idx = 1; idx < skinDatas.size(); ++idx)
	{
		skin = skinDatas[idx];
		auto& sNode = skinMap[skin.skinName]; 
		sNode.resize(skin.skinVertCount);
		for (int i = 0; i < sNode.size(); ++i)
		{
			sNode[i].skinVertIndex = skinBase[skin.skinVertData[i].skinVertIndex].skinVertIndex;
			sNode[i].skinVertPos = skin.skinVertData[i].skinVertPos;
		}
	}
}

std::string PMDManager::GetModelTexturePath(const std::string& modelpath, const char* texpath)
{
	auto spoint = modelpath.rfind("/");		// "/"を逆から探索 
	auto path = (modelpath.substr(0, spoint) + "/" + texpath);		// ﾊﾟｽの合成
	return path;
}

std::string PMDManager::GetExtension(const char* path)
{
	std::string s = path;
	size_t dpoint = s.rfind(".") + 1;		// "."の場所を探る
	return s.substr(dpoint);
}

std::pair<std::string, std::string> PMDManager::SplitFileName(const std::string& path, const char splitter)
{
	int idx = path.find(splitter);
	std::pair<std::string, std::string> ret;
	ret.first = path.substr(0, idx);
	ret.second = path.substr(idx + 1, path.length() - idx - 1);
	return ret;
}

std::wstring PMDManager::GetWideStringFromString(std::string& str)
{
	// 呼び出し1回目(文字列数を得る)
	auto bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, nullptr, 0);

	// stringのwchar_t版, 得られた文字列数でリサイズしておく
	std::wstring wstr;
	wstr.resize(bsize);

	// 呼び出し2回目(確保済のwstrに変換文字列をコピー)
	bsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(), -1, &wstr[0], bsize);

	return wstr;
}

std::string PMDManager::GetToonPathFromIndex(const std::string& folder, int idx)
{
	std::string filename = toonTexNames[idx];
	std::string path = "toon/";
	path += filename;

	// ファイルシステムオブジェクトへのパスが有効かどうかを判断する
	if (PathFileExistsA(path.c_str()))
	{
		return path;
	}
	else
	{
		return folder + filename;
	}
}

void PMDManager::RecursiveMatrixMultply(BoneNode& node, DirectX::XMMATRIX& inMat)
{
	boneMatrices[node.boneIdx] *= inMat;
	for (auto& cnode : node.children)
	{
		RecursiveMatrixMultply(*cnode, boneMatrices[node.boneIdx]);
	}
}

void PMDManager::RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& quaternion)
{
	auto& bonenode = boneMap[bonename];
	auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);	// XMLoadFloat3...XMFLOAT3をXMVECTORに変換する
	auto q = DirectX::XMLoadFloat4(&quaternion);

	boneMatrices[bonenode.boneIdx] = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1)) 
										* DirectX::XMMatrixRotationQuaternion(q) * DirectX::XMMatrixTranslationFromVector(vec);
}

void PMDManager::RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT4& nextq, const float& t)
{
	auto& bonenode = boneMap[bonename];
	auto vec = DirectX::XMLoadFloat3(&bonenode.startPos);
	auto quaternion = XMLoadFloat4(&q);
	auto quaternion2 = XMLoadFloat4(&nextq);

	boneMatrices[bonenode.boneIdx] = DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1)) *
		DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(quaternion, quaternion2, t)) *
		DirectX::XMMatrixTranslationFromVector(vec);
}

void PMDManager::Transformation(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& nextpos, float t)
{
	DirectX::XMFLOAT3 p;
	p.x = (nextpos.x - pos.x) * t + pos.x;
	p.y = (nextpos.y - pos.y) * t + pos.y;
	p.z = (nextpos.z - pos.z) * t + pos.z;

	auto& center = boneMap["センター"].boneIdx;
	boneMatrices[center] *= DirectX::XMMatrixTranslation(p.x / 3, p.y / 3, p.z / 3);
}

void PMDManager::MotionUpdate(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, const int& frame)
{
	// 最初に初期化
	std::fill(boneMatrices.begin(), boneMatrices.end(), DirectX::XMMatrixIdentity());

	// ポージング適用
	for (auto& boneAnim : animationdata)
	{
		auto& keyframe = boneAnim.second;

		// ラムダ式
		auto frameIt = std::find_if(keyframe.rbegin(), keyframe.rend(),
			[frame](const BoneKeyFrames& k) {return k.frameNo <= frame; });	// 現在のフレームに近い前のフレーム

		if (frameIt == keyframe.rend())
		{
			// 対象のものがなかったらendが返ってくる
			// 対象のものがなければやり直し
			continue;
		}
		auto nextFrameIt = frameIt.base();	// 現在のフレームに近い次のフレーム
		
		// モデルの中に対象の骨がなければ以下の処理を通さない
		// std::mapの仕様でない骨が作られてしまい、予想外の挙動を起こすため
		auto name = boneMap.find(boneAnim.first.c_str());
		if (name == boneMap.end())
		{
			continue;
		}
		
		// 現在のフレームと次のフレームが同じならそのまま回す
		if (nextFrameIt == keyframe.end())
		{
			// 骨回す
			RotateBone(boneAnim.first.c_str(), frameIt->quaternion);
		}
		else
		{
			// 違う場合はポーズの差を補完した状態にする
			float a = (float)frameIt->frameNo;
			float b = (float)nextFrameIt->frameNo;
			float t = (static_cast<float>(frame - a)) / (b - a);	// 補間

			RotateBone(boneAnim.first.c_str(), frameIt->quaternion, nextFrameIt->quaternion, t);
			// 移動
			Transformation(frameIt->pos, nextFrameIt->pos, t);
		}
	}

	// ツリーをトラバース
	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();

	RecursiveMatrixMultply(boneMap["センター"], rootmat);

	// マップを更新
	std::copy(boneMatrices.begin(), boneMatrices.end(), matMap);
}

void PMDManager::ChangeSkin(const std::string& skinname, const float& weight)
{
	auto data = skinMap[skinname];
	for (auto& d : data)
	{
		vertexDatas[d.skinVertIndex].pos.x += d.skinVertPos.x * weight;
		vertexDatas[d.skinVertIndex].pos.y += d.skinVertPos.y * weight;
		vertexDatas[d.skinVertIndex].pos.z += d.skinVertPos.z * weight;
	}
}

void PMDManager::SkinUpdate(const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& frame)
{
	// 最初にbaseの表情に戻す
	auto data = skinMap["base"];
	for (auto& d : data)
	{
		vertexDatas[d.skinVertIndex].pos.x = d.skinVertPos.x;
		vertexDatas[d.skinVertIndex].pos.y = d.skinVertPos.y;
		vertexDatas[d.skinVertIndex].pos.z = d.skinVertPos.z;
	}

	for (auto& skinAnim : skindata)
	{
		auto& keyframe = skinAnim.second;

		// ラムダ式
		auto frameIt = std::find_if(keyframe.rbegin(), keyframe.rend(),
			[frame](const SkinKeyFrames& k) {return k.frameNo <= frame; });	// 現在のフレームに近い前のフレーム

		if (frameIt == keyframe.rend())
		{
			// 対象のものがなかったらendが返ってくる
			// 対象のものがなければやり直し
			continue;
		}
		auto nextFrameIt = frameIt.base();	// 現在のフレームに近い次のフレーム

		if (nextFrameIt == keyframe.end())
		{
			ChangeSkin(skinAnim.first.c_str(), frameIt->weight);
		}
		else
		{
			float f = (float)frameIt->frameNo;
			float nextf = (float)nextFrameIt->frameNo;
			float t = (static_cast<float>(frame - f)) / (nextf - f);	// 補間

			float weight = 0.0f;
			if (frameIt->weight == nextFrameIt->weight)
			{
				weight = frameIt->weight;
			}
			else
			{
				weight = (nextFrameIt->weight - frameIt->weight) * t + frameIt->weight;
			}

			ChangeSkin(skinAnim.first.c_str(), weight);
		}
	}

	D3D12_RANGE range = { 0,0 };
	PMDVertexData* vertexMap = nullptr;
	auto result = vertexBuffer->Map(0, &range, (void**)& vertexMap);
	std::copy(vertexDatas.begin(), vertexDatas.end(), vertexMap);
	vertexBuffer->Unmap(0, nullptr);
}

void PMDManager::Update(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, 
					const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& nowframe)
{
	MotionUpdate(animationdata, nowframe);
	SkinUpdate(skindata, nowframe);
}

void PMDManager::Draw(ID3D12GraphicsCommandList* cmdList)
{
	auto rgstDescriptorHeap = Dx12Constants::Instance().GetRgstDescriptorHeap();
	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	cmdList->SetGraphicsRootDescriptorTable(0, rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	// 骨をセット
	cmdList->SetDescriptorHeaps(1, &boneHeap);
	cmdList->SetGraphicsRootDescriptorTable(2, boneHeap->GetGPUDescriptorHandleForHeapStart());

	// モデルの描画
	unsigned int offset = 0;
	cmdList->SetDescriptorHeaps(1, &matDescriptorHeap);
	auto handle = matDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto hptr = Dx12Device::Instance().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto& mat : matDatas)
	{
		cmdList->SetGraphicsRootDescriptorTable(1, handle);
		handle.ptr += hptr * 5;
		cmdList->DrawIndexedInstanced(mat.faceVertCount, 1, offset, 0, 0);
		offset += mat.faceVertCount;
	}
}

void PMDManager::ShadowDraw(ID3D12GraphicsCommandList* cmdList)
{
	auto rgstDescriptorHeap = Dx12Constants::Instance().GetRgstDescriptorHeap();
	cmdList->SetDescriptorHeaps(1, &rgstDescriptorHeap);
	auto rgstHeapHandle = rgstDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cmdList->SetGraphicsRootDescriptorTable(0, rgstHeapHandle);

	cmdList->SetDescriptorHeaps(1, &boneHeap);
	cmdList->SetGraphicsRootDescriptorTable(1, boneHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);

	unsigned int offset = 0;
	for (auto m : matDatas)
	{
		// 描画
		cmdList->DrawIndexedInstanced(m.faceVertCount, 1, offset, 0, 0);
		offset += m.faceVertCount;
	}
}

const DirectX::XMFLOAT3& PMDManager::GetPos()
{
	auto& center = boneMap["センター"].boneIdx;
	
	return DirectX::XMFLOAT3(boneMatrices[center].r[3].m128_f32[0], boneMatrices[center].r[3].m128_f32[1], boneMatrices[center].r[3].m128_f32[2]);
}
