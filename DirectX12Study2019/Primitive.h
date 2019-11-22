#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

// �`���\���������
struct PrimitiveVertex
{
	DirectX::XMFLOAT3 pos;		// ���W
	DirectX::XMFLOAT3 normal;	// �@��
	DirectX::XMFLOAT2 uv;		// UV

	PrimitiveVertex();
	PrimitiveVertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _normal, DirectX::XMFLOAT2 _uv);
};

// �V���v���Ȍ`�̐e�N���X
class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual void Draw(ID3D12GraphicsCommandList* cmdlist) = 0;
};

