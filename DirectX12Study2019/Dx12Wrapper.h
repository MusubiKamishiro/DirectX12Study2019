#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class PMDManager;
class VMDLoader;


struct Vector3
{
	float x, y, z;
};

struct Vertex
{
	Vertex() {};
	Vertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT2 _uv) : pos(_pos), uv(_uv) {};

	DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3(0, 0, 0);	// 座標
	DirectX::XMFLOAT2 uv = DirectX::XMFLOAT2(0, 0);		// UV座標
};


class Dx12Wrapper
{
private:
	IDXGISwapChain4* swapChain = nullptr;	// スワップチェイン
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

	// 深度バッファの作成
	void CreateDepthBuff();
	ID3D12Resource* depthBuff = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;

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

	// PMD関連
	std::string modelPath;	// モデルのパス
	std::shared_ptr<PMDManager> pmdManager;

	// VMD
	std::shared_ptr<VMDLoader> vmdLoader;
	std::string vmdPath;	// vmdのパス

	unsigned int startTime;
	unsigned int frame = 0;
	
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

