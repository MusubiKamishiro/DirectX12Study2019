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
	// ルートシグネチャの初期化
	void InitPrimitiveRootSignature();
	ID3D12RootSignature* primitiveRootSignature = nullptr;
	// パイプラインステートの初期化
	void InitPrimitivePipelineState();
	ID3D12PipelineState* primitivePipelineState = nullptr;

public:
	PrimitiveManager();
	~PrimitiveManager();

	// プリミティブ用のPSOのセット
	// Primitive系のDraw()命令をする前に必ず呼ぶ
	void SetPrimitiveDrawMode(ID3D12GraphicsCommandList* cmdlist);

	// 平面オブジェクトの作成
	Plane* CreatePlane(DirectX::XMFLOAT3 pos, float width, float depth);
};

