#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

// 形を構成するもの
struct PrimitiveVertex
{
	DirectX::XMFLOAT3 pos;		// 座標
	DirectX::XMFLOAT3 normal;	// 法線
	DirectX::XMFLOAT2 uv;		// UV

	PrimitiveVertex();
	PrimitiveVertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _normal, DirectX::XMFLOAT2 _uv);
};

// シンプルな形の親クラス
class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual void Draw(ID3D12GraphicsCommandList* cmdlist) = 0;
};

