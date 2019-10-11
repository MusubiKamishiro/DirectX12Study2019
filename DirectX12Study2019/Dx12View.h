#pragma once
#include <d3d12.h>

#include <vector>


class Dx12View
{
private:
	ID3D12Resource* vertexBuffer = nullptr;	// 頂点バッファ
	ID3D12Resource* indexBuffer = nullptr;	// インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// インデックスバッファビュー

public:
	Dx12View(ID3D12Device& dev, const std::vector<char>& vertices, const std::vector<unsigned short>& indices);
	~Dx12View();
};

