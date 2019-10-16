#include "Dx12Device.h"

#include <vector>
#include <string>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")


Dx12Device::Dx12Device()
{
	// DXGI�t�@�N�g���̍쐬
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	// �t�B�[�`���[���x���̑I��
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	// �O���t�B�b�N�X�A�_�v�^��񋓂�����
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(adapter);
	}
	// ���̒�����NVIDIA��T��
	for (auto& adpt : adapters)
	{
		DXGI_ADAPTER_DESC aDesc = {};
		adpt->GetDesc(&aDesc);
		std::wstring strDesc = aDesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			// NVIDIA�A�_�v�^������
			adapter = adpt;
			break;
		}
	}

	// ���x���̍������̂��猟�؂��A�����������x����K�p����
	for (auto& l : levels)
	{
		// �f�B�X�v���C�A�_�v�^�[��\���f�o�C�X�̍쐬
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
