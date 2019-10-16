#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

class Dx12Device
{
private:
	Dx12Device();
	Dx12Device(const Dx12Device&);		// コピー禁止
	void operator=(const Dx12Device&);	// 代入禁止

	// 基本的な奴(DXGI)
	IDXGIFactory6* dxgiFactory = nullptr;
	// 基本的な奴(デバイス)
	ID3D12Device* device = nullptr;

public:
	static Dx12Device& Instance();
	~Dx12Device();

	ID3D12Device* GetDevice()const;
	IDXGIFactory6* GetDxgiFactory()const;
};

