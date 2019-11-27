#include "PrimitiveManager.h"
#include <d3dcompiler.h>
#include "d3dx12.h"
#include "Dx12Device.h"
#include "Plane.h"

PrimitiveManager::PrimitiveManager()
{
	auto result = D3DCompileFromFile(L"PrimitiveShader.hlsl", nullptr, nullptr, "vs", "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primitiveVertexShader, nullptr);
	result = D3DCompileFromFile(L"PrimitiveShader.hlsl", nullptr, nullptr, "ps", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &primitivePixelShader, nullptr);

	InitPrimitiveRootSignature();
	InitPrimitivePipelineState();
}

PrimitiveManager::~PrimitiveManager()
{
	primitiveVertexShader->Release();
	primitivePixelShader->Release();
	primitivePipelineState->Release();
	primitiveRootSignature->Release();
}

void PrimitiveManager::SetPrimitiveDrawMode(ID3D12GraphicsCommandList* cmdlist)
{
	cmdlist->SetPipelineState(primitivePipelineState);
	cmdlist->SetGraphicsRootSignature(primitiveRootSignature);
}

Plane* PrimitiveManager::CreatePlane(DirectX::XMFLOAT3 pos, float width, float depth)
{
	return new Plane(pos, width, depth);
}

void PrimitiveManager::InitPrimitiveRootSignature()
{
	D3D12_DESCRIPTOR_RANGE descRange[3] = {};
	// "b0"	wvp
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[0].BaseShaderRegister = 0;	// ���W�X�^�ԍ�
	descRange[0].NumDescriptors = 1;		// 1��œǂސ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "t0"	�e
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[1].BaseShaderRegister = 0;	// ���W�X�^�ԍ�
	descRange[1].NumDescriptors = 1;		// 1��œǂސ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// "t1" �e�N�X�`��
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[2].BaseShaderRegister = 1;	// ���W�X�^�ԍ�
	descRange[2].NumDescriptors = 1;		// 1��œǂސ�
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;			// �����W�̐�
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];	// �Ή����郌���W�ւ̃|�C���^
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;			// �����W�̐�
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];	// �Ή����郌���W�ւ̃|�C���^
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;			// �����W�̐�
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[2];	// �Ή����郌���W�ւ̃|�C���^
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	
	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;	// ���ʂȃt�B���^���g�p���Ȃ�
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;		// MIPMAP����Ȃ�
	samplerDesc.MinLOD = 0.0f;					// MIPMAP�����Ȃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;	// �G�b�W�̂���(������)
	samplerDesc.ShaderRegister = 0;				// �g�p����V�F�[�_���W�X�^(�X���b�g)
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// �S���̃f�[�^���V�F�[�_�Ɍ�����
	samplerDesc.RegisterSpace = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.NumParameters = 3;
	rsd.NumStaticSamplers = 1;
	rsd.pParameters = &rootParam[0];
	rsd.pStaticSamplers = &samplerDesc;

	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	auto result = D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

	// ���[�g�V�O�l�`���̍쐬
	result = Dx12Device::Instance().GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&primitiveRootSignature));
}

void PrimitiveManager::InitPrimitivePipelineState()
{
	// ���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC layouts[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
												D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	// �p�C�v���C���X�e�[�g�����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	// ���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = primitiveRootSignature;
	gpsDesc.InputLayout.pInputElementDescs = layouts;
	gpsDesc.InputLayout.NumElements = _countof(layouts);
	// �V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(primitiveVertexShader);// ���_�V�F�[�_
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(primitivePixelShader);	// �s�N�Z���V�F�[�_
	// �����_�^�[�Q�b�g
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;		// ���̃^�[�Q�b�g���Ɛݒ肷��t�H�[�}�b�g����
	gpsDesc.NumRenderTargets = 1;							// ��v�����Ă���
	// �[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	// ���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;	// �\�ʂ�������Ȃ��āA���ʂ��`�悷��悤�ɂ���
	// ���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;	// �S��1
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// �O�p�`

	auto result = Dx12Device::Instance().GetDevice()->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&primitivePipelineState));
}