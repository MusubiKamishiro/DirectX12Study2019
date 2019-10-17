#include "ImageManager.h"

#include "Dx12Device.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib, "DirectXTex.lib")


std::string ImageManager::GetExtension(const char* path)
{
	std::string s = path;
	size_t dpoint = s.rfind(".") + 1;	// "."の場所を探る
	return s.substr(dpoint);
}

std::wstring ImageManager::GetWideStringFromString(const std::string& str)
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

ID3D12Resource* ImageManager::CreateTextureResource(ID3D12Resource* buff, const unsigned int width, const unsigned int height, const unsigned int arraySize)
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapprop.CreationNodeMask = 1;
	heapprop.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Alignment = 0;
	resDesc.Width = width;
	resDesc.Height = height;
	resDesc.DepthOrArraySize = arraySize;
	resDesc.MipLevels = 0;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	auto result = Dx12Device::Instance().GetDevice()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&buff));

	if (result == S_OK)
	{
		return buff;
	}
	return nullptr;
}

ImageManager::ImageManager()
{
	auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

ImageManager::~ImageManager()
{
	for (auto& t : table)
	{
		t.second->Release();
	}
}

ID3D12Resource* ImageManager::Load(const std::string& filepath)
{
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage img;
	ID3D12Resource* texBuff = nullptr;
	auto result = E_FAIL;

	// データが見つからなかったら読み込む
	auto it = table.find(filepath);
	if (it == table.end())
	{
		auto ext = GetExtension(filepath.c_str());
		auto path = GetWideStringFromString(filepath);

		// 画像読み込み
		if (ext == "png" || ext == "bmp" || ext == "jpg" || ext == "spa" || ext == "sph")
		{
			result = DirectX::LoadFromWICFile(path.data(), DirectX::WIC_FLAGS_NONE, &metadata, img);
		}
		else if (ext == "tga")
		{
			result = DirectX::LoadFromTGAFile(path.data(), &metadata, img);
		}

		// テクスチャバッファの作成
		texBuff = CreateTextureResource(texBuff, metadata.width, metadata.height, metadata.arraySize);

		// テクスチャバッファに書き込み
		result = texBuff->WriteToSubresource(0, nullptr, img.GetPixels(), metadata.width * 4, img.GetPixelsSize());

		// テーブルに追加しておく
		table.emplace(filepath, texBuff);

		return texBuff;
	}
	else
	{
		return table[filepath];
	}
}
