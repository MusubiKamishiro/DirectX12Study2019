#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

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

public:
	static Dx12Constants& Instance();
	~Dx12Constants();

	ID3D12DescriptorHeap* GetRgstDescriptorHeap()const;
	WVP* GetMappedMatrix()const;
};

