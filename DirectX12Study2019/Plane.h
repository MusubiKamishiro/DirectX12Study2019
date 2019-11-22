#pragma once
#include "Primitive.h"

// éläpå`
class Plane : public Primitive
{
private:
	ID3D12Resource* vertexBuff;
	D3D12_VERTEX_BUFFER_VIEW vbView = {};

public:
	Plane(const DirectX::XMFLOAT3& pos, float width, float depth);
	~Plane();

	void Draw(ID3D12GraphicsCommandList* cmdlist);
};

