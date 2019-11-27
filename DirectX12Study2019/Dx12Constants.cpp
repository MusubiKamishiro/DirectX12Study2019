#include "Dx12Constants.h"
#include "d3dx12.h"
#include <algorithm>

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
	auto eyePos = DirectX::XMFLOAT3(0, 20, -30);	// �J�����̈ʒu(���_)
	auto focusPos = DirectX::XMFLOAT3(0, 10, 0);	// �œ_�̈ʒu(�����_)
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// �J�����̏����(�ʏ��(0.0f, 1.0f, 0.0f))	// �J�������Œ肷�邽�߂̂���

	// DirectX::XMMatrixLookAtLH�̒��g
	/*DirectX::XMVECTOR EyeDirection = DirectX::XMVectorSubtract(XMLoadFloat3(&focusPos), XMLoadFloat3(&eyePos));

	DirectX::XMVECTOR R2 = DirectX::XMVector3Normalize(EyeDirection);

	DirectX::XMVECTOR R0 = DirectX::XMVector3Cross(XMLoadFloat3(&up), R2);
	R0 = DirectX::XMVector3Normalize(R0);

	DirectX::XMVECTOR R1 = DirectX::XMVector3Cross(R2, R0);

	DirectX::XMVECTOR NegEyePosition = DirectX::XMVectorNegate(XMLoadFloat3(&eyePos));

	DirectX::XMVECTOR D0 = DirectX::XMVector3Dot(R0, NegEyePosition);
	DirectX::XMVECTOR D1 = DirectX::XMVector3Dot(R1, NegEyePosition);
	DirectX::XMVECTOR D2 = DirectX::XMVector3Dot(R2, NegEyePosition);

	DirectX::XMMATRIX M;
	M.r[0] = DirectX::XMVectorSelect(D0, R0, g_XMSelect1110.v);
	M.r[1] = DirectX::XMVectorSelect(D1, R1, g_XMSelect1110.v);
	M.r[2] = DirectX::XMVectorSelect(D2, R2, g_XMSelect1110.v);
	M.r[3] = g_XMIdentityR3.v;

	M = XMMatrixTranspose(M);*/
	// �����܂Œ��g
	//DirectX::XMMATRIX camera = M;
	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&eyePos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));	// �J�����s��
													// XMLoadFloat3...XMFloat3��XMVECTOR�ɕϊ�����

	auto aspect = (float)wsize.width / (float)wsize.height;		// �r���[��Ԃ̍����ƕ��̃A�X�y�N�g��
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(3.1415f / 2.0f, aspect, 0.5f, 300.0f);		// �ˉe�s��	// LH...LeftHand�̗�,RH�������
	DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicLH(30, 30, 0.5f, 300.0f);

	mappedMatrix.viewProj = camera * projection;	// �����鏇�Ԃɂ͋C��t���悤
	mappedMatrix.wvp = world * camera * projection;

	auto lightPos = DirectX::XMFLOAT3(50, 70, -15);
	DirectX::XMMATRIX lcamera = DirectX::XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&focusPos), XMLoadFloat3(&up));
	mappedMatrix.lightVP = lcamera * lightProj;

	size_t size = sizeof(mappedMatrix);
	size = (size + 0xff) & ~0xff;		// 256�A���C�����g�ɍ��킹�Ă���

	result = dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuff));

	result = constBuff->Map(0, nullptr, (void**)& m);	// �V�F�[�_�ɑ���
	*m = mappedMatrix;

	auto handle = rgstDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;
	dev->CreateConstantBufferView(&cbvDesc, handle);
}

void Dx12Constants::CameraMove(const VMDCameraData& cameraData, const VMDCameraData& nextCameraData, const float& t)
{
	// �J�����̐ݒ�
	DirectX::XMFLOAT3 focusPos;	// �œ_�̈ʒu(�����_)
	focusPos.x = (nextCameraData.location.x - cameraData.location.x) * t + cameraData.location.x;
	focusPos.y = (nextCameraData.location.y - cameraData.location.y) * t + cameraData.location.y;
	focusPos.z = (nextCameraData.location.z - cameraData.location.z) * t + cameraData.location.z;
	
	DirectX::XMFLOAT3 rotation;	// �J�����̊p�x
	rotation.x = (nextCameraData.rotation.x - cameraData.rotation.x) * t + cameraData.rotation.x;
	rotation.y = (nextCameraData.rotation.y - cameraData.rotation.y) * t + cameraData.rotation.y;
	rotation.z = (nextCameraData.rotation.z - cameraData.rotation.z) * t + cameraData.rotation.z;
	
	auto lenght = (nextCameraData.length - cameraData.length) * t + cameraData.length;	// �����_����̋���
	//auto eyePos = DirectX::XMFLOAT3(focusPos.x + lenght * rotation.x, focusPos.y + lenght * rotation.y, focusPos.z + lenght * rotation.z);	// �J�����̈ʒu(���_)
	auto eyePos = DirectX::XMFLOAT3(focusPos.x, focusPos.y, focusPos.z + lenght);	// �J�����̈ʒu(���_)

	// ������0�̎���location���J�����̍��W�ɂȂ�
	if (lenght == 0.00f)
	{
		focusPos = DirectX::XMFLOAT3(0, focusPos.y, 0);	// �œ_�̈ʒu(�����_)
	}
	auto up = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);	// �J�����̏����(�ʏ��(0.0f, 1.0f, 0.0f))	// �J�������Œ肷�邽�߂̂���

	DirectX::XMMATRIX camera = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eyePos), DirectX::XMLoadFloat3(&focusPos), DirectX::XMLoadFloat3(&up));	// �J�����s��

	auto wsize = Application::Instance().GetWindowSize();	// ��ʃT�C�Y
	auto aspect = (float)wsize.width / (float)wsize.height;	// �r���[��Ԃ̍����ƕ��̃A�X�y�N�g��
																		// ����p
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH((3.1415f / 180.0f) * cameraData.viewIngAngle, aspect, 0.5f, 300.0f);		// �ˉe�s��	// LH...LeftHand�̗�,RH�������

	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	mappedMatrix.world = world;
	mappedMatrix.viewProj = camera * projection;	// �����鏇�Ԃɂ͋C��t���悤
	mappedMatrix.wvp = world * camera * projection;

	*m = mappedMatrix;
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

void Dx12Constants::Update(const std::vector<VMDCameraData>& cameraData, const int& frame)
{
	// �����_��
	auto frameIt = std::find_if(cameraData.rbegin(), cameraData.rend(),
		[frame](const VMDCameraData& c) {return c.frameNo <= frame; });	// ���݂̃t���[���ɋ߂��O�̃t���[��

	if (frameIt == cameraData.rend())
	{
		// �Ώۂ̂��̂��Ȃ�������end���Ԃ��Ă���
		// �Ώۂ̂��̂��Ȃ���΂�蒼��
		return;
	}
	auto nextFrameIt = frameIt.base();	// ���݂̃t���[���ɋ߂����̃t���[��

	// ���݂̃t���[���Ǝ��̃t���[���������Ȃ炻�̂܂�
	if (nextFrameIt == cameraData.end())
	{
		//RotateBone(boneAnim.first.c_str(), frameIt->quaternion);
		//CameraMove(frame);
	}
	else
	{
		float a = (float)frameIt->frameNo;
		float b = (float)nextFrameIt->frameNo;
		float t = (static_cast<float>(frame - a)) / (b - a);	// ���
		
		CameraMove(*frameIt, *nextFrameIt, t);
	}


}

ID3D12DescriptorHeap* Dx12Constants::GetRgstDescriptorHeap() const
{
	return rgstDescriptorHeap;
}

WVP* Dx12Constants::GetMappedMatrix() const
{
	return m;
}
