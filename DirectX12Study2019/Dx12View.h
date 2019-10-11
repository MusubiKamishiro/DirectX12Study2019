#pragma once
#include <d3d12.h>

#include <vector>


class Dx12View
{
private:
	ID3D12Resource* vertexBuffer = nullptr;	// ���_�o�b�t�@
	ID3D12Resource* indexBuffer = nullptr;	// �C���f�b�N�X�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// ���_�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// �C���f�b�N�X�o�b�t�@�r���[

public:
	Dx12View(ID3D12Device& dev, const std::vector<char>& vertices, const std::vector<unsigned short>& indices);
	~Dx12View();
};

