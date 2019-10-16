#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Dx12Device
{
private:
	Dx12Device();
	Dx12Device(const Dx12Device&);		// �R�s�[�֎~
	void operator=(const Dx12Device&);	// ����֎~

	// ��{�I�ȓz(DXGI)
	IDXGIFactory6* dxgiFactory = nullptr;
	// ��{�I�ȓz(�f�o�C�X)
	ID3D12Device* device = nullptr;

public:
	static Dx12Device& Instance();
	~Dx12Device();

	ID3D12Device* GetDevice()const;
	IDXGIFactory6* GetDxgiFactory()const;
};

