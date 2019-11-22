#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

class Primitive;
class Plane;

class PrimitiveManager
{
private:
	ID3DBlob* primitiveVertexShader = nullptr;
	ID3DBlob* primitivePixelShader = nullptr;
	// ���[�g�V�O�l�`���̏�����
	void InitPrimitiveRootSignature();
	ID3D12RootSignature* primitiveRootSignature = nullptr;
	// �p�C�v���C���X�e�[�g�̏�����
	void InitPrimitivePipelineState();
	ID3D12PipelineState* primitivePipelineState = nullptr;

public:
	PrimitiveManager();
	~PrimitiveManager();

	// �v���~�e�B�u�p��PSO�̃Z�b�g
	// Primitive�n��Draw()���߂�����O�ɕK���Ă�
	void SetPrimitiveDrawMode(ID3D12GraphicsCommandList* cmdlist);

	// ���ʃI�u�W�F�N�g�̍쐬
	Plane* CreatePlane(DirectX::XMFLOAT3 pos, float width, float depth);
};

