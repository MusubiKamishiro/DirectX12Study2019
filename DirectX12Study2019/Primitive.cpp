#include "Primitive.h"

Primitive::Primitive()
{
}

Primitive::~Primitive()
{
}

PrimitiveVertex::PrimitiveVertex()
{
	pos = DirectX::XMFLOAT3(0, 0, 0);
	normal = DirectX::XMFLOAT3(0, 0, 0);
	uv = DirectX::XMFLOAT2(0, 0);
}

PrimitiveVertex::PrimitiveVertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT3 _normal, DirectX::XMFLOAT2 _uv)
{
	pos = _pos;
	normal = _normal;
	uv = _uv;
}
