#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>

#include <vector>

#include <DirectXMath.h>


struct Vector3
{
	float x, y, z;
};

struct Vertex {
	DirectX::XMFLOAT3 pos;		// 座標
	DirectX::XMFLOAT2 uv;		// UV座標
	//DirectX::XMFLOAT3 normal;	// 法線
};

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

	// デバッグレイヤの作成
	void CreateDebugLayer(HRESULT& result);
	// フィーチャーレベルの選択
	void InitFeatureLevel(HRESULT& result);
	// スワップチェインの作成
	void CreateSwapChain(HRESULT& result, HWND hwnd);
	// レンダーターゲットの作成
	void CreateRenderTarget(HRESULT& result);
	// 頂点バッファの作成
	void CreateVertexBuffer(HRESULT& result);

	// 画面の初期化
	void InitScreen();
	// シェーダの初期化
	void InitShader(HRESULT& result);
	ID3DBlob* vertexShader = nullptr;	// 頂点シェーダ
	ID3DBlob* pixelShader = nullptr;	// ピクセルシェーダ
	// ルートシグネチャの初期化
	void InitRootSignatur(HRESULT& result);
	ID3D12RootSignature* rootSignature = nullptr;	// これが最終的に欲しいオブジェクト
	// パイプラインステートの初期化
	void InitPipelineState(HRESULT& result);
	ID3D12PipelineState* pipelineState = nullptr;

	// コマンドキューに投げる
	void ExecuteCmd();
	// 待ち
	void WaitExecute();
	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// レンダーターゲットビュー用のヒープ
	std::vector<ID3D12Resource*> backBuffers;

	ID3D12Resource* vertexBuffer = nullptr;		// 頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// 頂点バッファビュー

	UINT64 fenceValue = 0;
	UINT bbIdx = 0;

	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// バリア
	

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

