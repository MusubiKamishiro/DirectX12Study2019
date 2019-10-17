#include "Dx12Constants.h"
#include "d3dx12.h"

#include "Application.h"
#include "Dx12Device.h"

#pragma comment(lib,"d3d12.lib")


Dx12Constants::Dx12Constants()
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto dev = Dx12Device::Instance().GetDevice();
	auto result = dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&rgstDescriptorHeap));

	auto wsize = Application::Instance().GetWindowSize();	// 画面サイズ

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	mappedMatrix.world = world;

	// カメラの設定
	auto eyePos = DirectX::XMFLOAT3(0, 20, -15);	// カメラの位置(視点)
	auto focusPos = DirectX::XMFLOAT3(0, 10, 0);	// 焦点の位置(注視点)
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// カメラの上方向(通常は(0.0f, 1.0f, 0.0f))	// カメラを固定するためのもの

	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));	// カメラ行列
													// XMLoadFloat3...XMFloat3をXMVECTORに変換する

	auto aspect = (float)wsize.width / (float)wsize.height;		// ビュー空間の高さと幅のアスペクト比
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(3.1415f / 2.0f, aspect, 0.5f, 300.0f);		// 射影行列	// LH...LeftHandの略,RHもあるよ
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicLH(30, 30, 0.5f, 300.0f);

	mappedMatrix.viewProj = camera * projection;	// かける順番には気を付けよう
	mappedMatrix.wvp = world * camera * projection;

	auto lightPos = DirectX::XMFLOAT3(50, 70, -15);
	DirectX::XMMATRIX _lcamera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));
	mappedMatrix.lightVP = _lcamera * lightProj;

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256アライメントに合わせている

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));

	result = constBuff->Map(0, nullptr, (void**)& m);	// シェーダに送る
	*m = mappedMatrix;

	auto handle = rgstDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;
	dev->CreateConstantBufferView(&cbvDesc, handle);
}

Dx12Constants& Dx12Constants::Instance()
{
	static Dx12Constants instance;
	return instance;
}

Dx12Constants::~Dx12Constants()
{
	rgstDescriptorHeap->Release();
	constBuff->Release();
}

ID3D12DescriptorHeap* Dx12Constants::GetRgstDescriptorHeap() const
{
	return rgstDescriptorHeap;
}

WVP* Dx12Constants::GetMappedMatrix() const
{
	return m;
}
