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
	DirectX::XMFLOAT3 pos;	// 座標
	DirectX::XMFLOAT2 uv;	// UV座標
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
	void CreateDebugLayer();
	// フィーチャーレベルの選択
	void InitFeatureLevel();
	// スワップチェインの作成
	void CreateSwapChain(HWND hwnd);
	// レンダーターゲットの作成
	void CreateRenderTarget();
	// 頂点バッファの作成
	void CreateVertexBuffer();

	// シェーダの初期化
	void InitShader();
	ID3DBlob* vertexShader = nullptr;	// 頂点シェーダ
	ID3DBlob* pixelShader = nullptr;	// ピクセルシェーダ
	D3D12_VIEWPORT viewport;	// ビューポート
	D3D12_RECT scissorRect;		// シザーレクト
	// ルートシグネチャの初期化
	void InitRootSignatur();
	ID3D12RootSignature* rootSignature = nullptr;	// これが最終的に欲しいオブジェクト
	// パイプラインステートの初期化
	void InitPipelineState();
	ID3D12PipelineState* pipelineState = nullptr;

	// テクスチャリソースの作成
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
												const unsigned int height = 4, const unsigned int arraySize = 1);

	// 命令のクリア
	void ClearCmd(ID3D12PipelineState* pipelinestate, ID3D12RootSignature* rootsignature);

	// バリアの解除
	void UnlockBarrier(ID3D12Resource* buffer);
	// バリアのセット
	void SetBarrier();

	// コマンドキューに投げる
	void ExecuteCmd();
	// 待ち
	void WaitExecute();
	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// レンダーターゲットビュー用のヒープ
	std::vector<ID3D12Resource*> backBuffers;

	ID3D12Resource* vertexBuffer = nullptr;	// 頂点バッファ
	ID3D12Resource* indexBuffer = nullptr;	// インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// インデックスバッファビュー

	UINT64 fenceValue = 0;
	UINT bbIdx = 0;
	
	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// バリア

	///ここから一時的な試験のための物
	///確認が終わり次第削除すること
	ID3D12Resource* texBuff = nullptr;
	ID3D12DescriptorHeap* texHeap = nullptr;
	void CreateTex();

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

