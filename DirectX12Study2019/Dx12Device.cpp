#include "Dx12Device.h"

#include <vector>
#include <string>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")


Dx12Device::Dx12Device()
{
	// DXGIファクトリの作成
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	// フィーチャーレベルの選択
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// グラフィックスアダプタを列挙させる
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(adapter);
	}
	// その中からNVIDIAを探す
	for (auto& adpt : adapters)
	{
		DXGI_ADAPTER_DESC aDesc = {};
		adpt->GetDesc(&aDesc);
		std::wstring strDesc = aDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			// NVIDIAアダプタを強制
			adapter = adpt;
			break;
		}
	}

	// レベルの高いものから検証し、成功したレベルを適用する
	for (auto& l : levels)
	{
		// ディスプレイアダプターを表すデバイスの作成
		auto result = D3D12CreateDevice(adapter, l, IID_PPV_ARGS(&device));

		if (SUCCEEDED(result))
		{
			break;
		}
	}
}


Dx12Device& Dx12Device::Instance()
{
	static Dx12Device instance;
	return instance;
}

Dx12Device::~Dx12Device()
{
	dxgiFactory->Release();
	device->Release();
}

ID3D12Device* Dx12Device::GetDevice() const
{
	return device;
}

IDXGIFactory6* Dx12Device::GetDxgiFactory() const
{
	return dxgiFactory;
}
