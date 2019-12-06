#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

#include "VMDLoader.h"

struct WVP
{
	DirectX::XMMATRIX world;		// ワールド
	DirectX::XMMATRIX viewProj;		// ビュープロジェクション
	DirectX::XMMATRIX wvp;			// 合成済み
	DirectX::XMMATRIX lightVP;		// ライトビュープロジェクション
};

//定数バッファの初期化
class Dx12Constants
{
private:
	Dx12Constants();
	Dx12Constants(const Dx12Constants&);	// コピー禁止
	void operator=(const Dx12Constants&);	// 代入禁止

	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;	// レジスタデスクリプタヒープ

	ID3D12Resource* constBuff = nullptr;	// 定数バッファ
	WVP* m = nullptr;
	WVP mappedMatrix;

	void CameraMove(const VMDCameraData& cameraData, const VMDCameraData& nextCameraData, const float& t);

	DirectX::XMFLOAT3 eyePos;	// カメラの位置(視点)
	DirectX::XMFLOAT3 focusPos;	// 焦点の位置(注視点)

public:
	static Dx12Constants& Instance();
	~Dx12Constants();

	void Update(const std::vector<VMDCameraData>& cameraData, const int& frame);

	ID3D12DescriptorHeap* GetRgstDescriptorHeap()const;
	WVP* GetMappedMatrix()const;

	const DirectX::XMFLOAT3& GetEyePos()const;
	const DirectX::XMFLOAT3& GetFocusPos()const;
};

