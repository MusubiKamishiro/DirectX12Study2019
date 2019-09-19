#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>


class Dx12Wrapper
{
private:
	// 基本的な奴(DXGI)
	IDXGIFactory6* dxgiFactory = nullptr;
	IDXGISwapChain4* swapChain = nullptr;	// スワップチェイン
	// 基本的な奴(デバイス)
	ID3D12Device* dev = nullptr;
	// コマンド系
	ID3D12CommandAllocator* cmdAllocator = nullptr;	// コマンドアロケータ
	// コマンドリスト本体
	ID3D12GraphicsCommandList* cmdList = nullptr;	// コマンドリスト
	// コマンド実行の単位
	ID3D12CommandQueue* cmdQueue = nullptr;			// コマンドキュー
	// 待ちのためのフェンス
	ID3D12Fence* fence = nullptr;

	// フィーチャーレベルの選択
	void InitFeatureLevel(HRESULT& result);
	// スワップチェインの作成
	void CreateSwapChain(HRESULT& result, HWND hwnd);
	// レンダーターゲットの作成
	void CreateRenderTarget(HRESULT& result);

	// 画面の初期化
	void InitScreen();
	// コマンドキューに投げる
	void ExecuteCmd();
	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// レンダーターゲットビュー用のヒープ
	std::vector<ID3D12Resource*> renderTargets;


public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
};

