#include "Dx12View.h"
//#include <DirectXTex.h>
#include "d3dx12.h"

#pragma comment(lib,"d3d12.lib")
//#pragma comment(lib, "DirectXTex.lib")


Dx12View::Dx12View(ID3D12Device& dev, const std::vector<char>& vertices, const std::vector<unsigned short>& indices)
{
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};
	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = vertices.size();	// 頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 頂点バッファの作成
	auto result = dev.CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE, &resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	// インデックスバッファの作成
	result = dev.CreateCommittedResource(&heapprop, D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	D3D12_RANGE range = { 0,0 };
	char* vertexMap = nullptr;
	result = vertexBuffer->Map(0, &range, (void**)& vertexMap);
	std::copy(vertices.begin(), vertices.end(), vertexMap);
	vertexBuffer->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(vertices)/* - 2*/;	// 頂点1つあたりのバイト数
	vbView.SizeInBytes = vertices.size();		// データ全体のサイズ

	unsigned short* ibuffptr = nullptr;
	result = indexBuffer->Map(0, &range, (void**)& ibuffptr);
	std::copy(indices.begin(), indices.end(), ibuffptr);
	indexBuffer->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();	// バッファの場所
	ibView.Format = DXGI_FORMAT_R16_UINT;	// フォーマット(shortなのでR16)
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);		// 総サイズ
}

Dx12View::~Dx12View()
{
}
