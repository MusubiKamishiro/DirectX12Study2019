#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

struct WVP
{
	DirectX::XMMATRIX world;		// ���[���h
	DirectX::XMMATRIX viewProj;		// �r���[�v���W�F�N�V����
	DirectX::XMMATRIX wvp;			// �����ς�
	DirectX::XMMATRIX lightVP;		// ���C�g�r���[�v���W�F�N�V����
};

//�萔�o�b�t�@�̏�����
class Dx12Constants
{
private:
	Dx12Constants();
	Dx12Constants(const Dx12Constants&);	// �R�s�[�֎~
	void operator=(const Dx12Constants&);	// ����֎~

	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;	// ���W�X�^�f�X�N���v�^�q�[�v

	ID3D12Resource* constBuff = nullptr;	// �萔�o�b�t�@
	WVP* m = nullptr;
	WVP mappedMatrix;

public:
	static Dx12Constants& Instance();
	~Dx12Constants();

	ID3D12DescriptorHeap* GetRgstDescriptorHeap()const;
	WVP* GetMappedMatrix()const;
};
