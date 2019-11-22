#include "Plane.h"
#include <array>
#include "d3dx12.h"
#include "Dx12Device.h"

Plane::Plane(const DirectX::XMFLOAT3& pos, float width, float depth)
{
	std::array<PrimitiveVertex, 4> vertices;
	vertices[0] = PrimitiveVertex(DirectX::XMFLOAT3(pos.x - width / 2, pos.y, pos.z - depth / 2), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(0, 1));	// ¶‰º
	vertices[1] = PrimitiveVertex(DirectX::XMFLOAT3(pos.x - width / 2, pos.y, pos.z + depth / 2), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(0, 0));	// ¶ã
	vertices[2] = PrimitiveVertex(DirectX::XMFLOAT3(pos.x + width / 2, pos.y, pos.z - depth / 2), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(1, 1));	// ‰Eã
	vertices[3] = PrimitiveVertex(DirectX::XMFLOAT3(pos.x + width / 2, pos.y, pos.z + depth / 2), DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT2(1, 0));	// ‰E‰º

	HRESULT result = Dx12Device::Instance().GetDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBuff));

	D3D12_RANGE range = { 0,0 };
	PrimitiveVertex* vbuffptr = nullptr;
	result = vertexBuff->Map(0, &range, (void**)& vbuffptr);
	std::copy(std::begin(vertices), std::end(vertices), vbuffptr);
	vertexBuff->Unmap(0, nullptr);

	vbView.BufferLocation = vertexBuff->GetGPUVirtualAddress();
	vbView.SizeInBytes = sizeof(vertices);
	vbView.StrideInBytes = sizeof(PrimitiveVertex);
}

Plane::~Plane()
{
	vertexBuff->Release();
}

void Plane::Draw(ID3D12GraphicsCommandList* cmdlist)
{
	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdlist->IASetVertexBuffers(0, 1, &vbView);
	cmdlist->DrawInstanced(4, 1, 0, 0);
}
