#pragma once
#include <d3d12.h>
#include <DirectXTex.h>

#include <map>
#include <string>


class ImageManager
{
private:
	std::map<std::string, ID3D12Resource*> table;

	// 拡張子を取得する
	//@param path 対象パスの文字列
	//@return 拡張子
	std::string GetExtension(const char* path);

	// string(マルチバイト文字列)からwstring(ワイド文字列)を得る
	//@param str マルチバイト文字列
	//@return 変換されたワイド文字列
	std::wstring GetWideStringFromString(const std::string& str);

	// テクスチャリソースの作成
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
		const unsigned int height = 4, const unsigned int arraySize = 1);

public:
	ImageManager();
	~ImageManager();

	ID3D12Resource* Load(const std::string& filepath);
};

