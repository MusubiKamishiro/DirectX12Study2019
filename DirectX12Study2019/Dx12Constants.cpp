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

	auto wsize = Application::Instance().GetWindowSize();	// ��ʃT�C�Y

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	mappedMatrix.world = world;

	// �J�����̐ݒ�
	auto eyePos = DirectX::XMFLOAT3(0, 20, -15);	// �J�����̈ʒu(���_)
	auto focusPos = DirectX::XMFLOAT3(0, 10, 0);	// �œ_�̈ʒu(�����_)
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// �J�����̏����(�ʏ��(0.0f, 1.0f, 0.0f))	// �J�������Œ肷�邽�߂̂���

	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));	// �J�����s��
													// XMLoadFloat3...XMFloat3��XMVECTOR�ɕϊ�����

	auto aspect = (float)wsize.width / (float)wsize.height;		// �r���[��Ԃ̍����ƕ��̃A�X�y�N�g��
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(3.1415f / 2.0f, aspect, 0.5f, 300.0f);		// �ˉe�s��	// LH...LeftHand�̗�,RH�������
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicLH(30, 30, 0.5f, 300.0f);

	mappedMatrix.viewProj = camera * projection;	// �����鏇�Ԃɂ͋C��t���悤
	mappedMatrix.wvp = world * camera * projection;

	auto lightPos = DirectX::XMFLOAT3(50, 70, -15);
	DirectX::XMMATRIX _lcamera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));
	mappedMatrix.lightVP = _lcamera * lightProj;

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256�A���C�����g�ɍ��킹�Ă���

	result = dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff));

	result = constBuff->Map(0, nullptr, (void**)& m);	// �V�F�[�_�ɑ���
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
