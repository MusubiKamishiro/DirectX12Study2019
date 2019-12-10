#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class PMDManager;
class VMDLoader;
class PrimitiveManager;
class Plane;
class ImageManager;

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
	std::shared_ptr<ImageManager> imageManager;

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
	

	// シェーダの初期化
	void InitShader();
	ID3DBlob* vertexShader = nullptr;	// 頂点シェーダ
	ID3DBlob* pixelShader = nullptr;	// ピクセルシェーダ
	D3D12_VIEWPORT viewport;	// ビューポート
	D3D12_RECT scissorRect;		// シザーレクト
	// ルートシグネチャの初期化
	void InitRootSignature();
	ID3D12RootSignature* rootSignature = nullptr;
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
	void UnlockBarrier(ID3D12Resource* buffer, const D3D12_RESOURCE_STATES& before, const D3D12_RESOURCE_STATES& after);
	// バリアのセット
	void SetBarrier(const D3D12_RESOURCE_STATES& before, const D3D12_RESOURCE_STATES& after);

	// コマンドキューに投げる
	void ExecuteCmd();
	// 待ち
	void WaitExecute();

	// 影用バッファの作成
	void CreateShadowBuff();
	ID3D12Resource* shadowBuff = nullptr;
	ID3D12DescriptorHeap* shadowDsvHeap;
	ID3D12DescriptorHeap* shadowSrvHeap;
	D3D12_VIEWPORT shadowViewport;	// 影用ビューポート
	D3D12_RECT shadowScissorRect;	// 影用シザーレクト
	// 影用シェーダー
	void InitShadowShader();
	// 影用ルートシグネチャの初期化
	void InitShadowRootSignature();
	ID3D12RootSignature* shadowRootSignature;
	// 影用パイプラインステートの初期化
	void InitShadowPipelineState();
	ID3D12PipelineState* shadowPipelineState;
	ID3DBlob* shadowVertexShader = nullptr;	// 頂点シェーダ
	// ライトからの撮影
	void CreateLightView();

	// 1パス目の作成
	void CreateFirstPassBuff();
	ID3D12DescriptorHeap* heapFor1stPassRTV;
	ID3D12DescriptorHeap* heapFor1stPassSRV;
	ID3D12Resource* firstPassBuff;

	// スクリーン(ペラポリ)テクスチャ作成
	void CreateScreenTexture();
	ID3D12Resource* screenVertexBuffer = nullptr;	// 頂点バッファ
	ID3D12Resource* screenIndexBuffer = nullptr;	// インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW svbView = {};			// 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW sibView = {};			// インデックスバッファビュー

	// 最終描画用のルートシグネチャ作成
	void InitLastRootSignature();
	ID3D12RootSignature* lastRootSignature = nullptr;
	// 最終描画用パイプラインステートの初期化
	void InitLastPipelineState();
	ID3D12PipelineState* lastPipelineState = nullptr;

	// 最終描画用シェーダーの初期化
	void InitLastShader();
	ID3DBlob* lastVertexShader = nullptr;	// 頂点シェーダ
	ID3DBlob* lastPixelShader = nullptr;	// ピクセルシェーダ

	
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;	// レンダーターゲットビュー用のヒープ
	std::vector<ID3D12Resource*> backBuffers;

	UINT64 fenceValue = 0;
	UINT bbIdx = 0;
	
	D3D12_RESOURCE_BARRIER BarrierDesc = {};	// バリア

	// ルートシグネチャの作成
	ID3D12RootSignature* CreateRootSignature(ID3D12RootSignature* rootSignature, const std::vector<D3D12_ROOT_PARAMETER>& rootParam, const D3D12_TEXTURE_ADDRESS_MODE& addressMode);

	// PMD関連
	std::vector<std::shared_ptr<PMDManager>> pmdManagers;

	// VMD
	std::vector<std::shared_ptr<VMDLoader>> vmdLoaders;

	// カメラVMD
	std::shared_ptr<VMDLoader> vmdCamera;
	std::string cameraPath;

	unsigned int startTime;
	int frame = 0;
	int maxFrame = 0;

	// 床
	std::shared_ptr<PrimitiveManager> primitiveManager;
	std::shared_ptr<Plane> plane;
	ID3D12Resource* floorImgBuff = nullptr;
	ID3D12DescriptorHeap* floorImgHeap = nullptr;

	// Effekseer
	void EffekseerInit();
	EffekseerRenderer::Renderer* efkRenderer;					// レンダラー
	Effekseer::Manager* efkManager;								// マネージャー
	EffekseerRenderer::SingleFrameMemoryPool* efkMemoryPool;	// メモリプール
	EffekseerRenderer::CommandList* efkCmdList;					// コマンドリスト
	Effekseer::Effect* effect;		// エフェクト
	Effekseer::Handle efkHandle;

	// imgui関係
	void ImGuiInit(HWND hwnd);
	ID3D12DescriptorHeap* imguiHeap;
	void ImGuiDraw();

	bool motionPlayFlag = false;
	bool motionReversePlayFlag = false;
	
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

