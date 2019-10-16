#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>
#include <array>
#include <string>
#include <memory>

class ImageManager;
class PMDLoader;


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

struct WVP
{
	DirectX::XMMATRIX world;		// ワールド
	DirectX::XMMATRIX viewProj;		// ビュープロジェクション
	DirectX::XMMATRIX wvp;			// 合成済み
	DirectX::XMMATRIX lightVP;		// ライトビュープロジェクション
};


class Dx12Wrapper
{
private:
	ID3D12DescriptorHeap* rgstDescriptorHeap = nullptr;	// レジスタデスクリプタヒープ
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

	// テクスチャリソースの作成
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
												const unsigned int height = 4, const unsigned int arraySize = 1);

	// 深度バッファの作成
	void CreateDepthBuff();
	ID3D12Resource* depthBuff = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;

	// 定数バッファの初期化
	void InitConstants();
	ID3D12Resource* constBuff = nullptr;	// 定数バッファ
	WVP* m = nullptr;
	WVP mappedMatrix;

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

	std::shared_ptr<ImageManager> imageManager;

	// 拡張子を取得する
	//@param path 対象パスの文字列
	//@return 拡張子
	std::string GetExtension(const char* path);

	//テクスチャのパスをセパレータ文字で分離する
	//@param path 対象のパス文字列
	//@param splitter 区切り文字
	//@return 分離前後の文字列ペア
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

	// PMD関連
	std::string modelPath;	// モデルのパス
	// PMDを読み込み
	std::shared_ptr<PMDLoader> pmdLoader;
	
	// マテリアルの初期化
	void InitMaterials();
	ID3D12DescriptorHeap* matDescriptorHeap;	// PMDマテリアル用デスクリプタヒープ
	std::vector<ID3D12Resource*> materialBuffs;	// PMDマテリアル用バッファ(マテリアル1つにつき1個)
	// モデルのテクスチャの作成
	void CreateModelTexture();
	std::vector<ID3D12Resource*> modelTexBuff;	// 通常テクスチャ
	std::vector<ID3D12Resource*> spaBuff;		// 加算テクスチャ
	std::vector<ID3D12Resource*> sphBuff;		// 乗算テクスチャ
	// 白テクスチャ作成
	void CreateWhiteTexture();
	ID3D12Resource* whiteTexBuff;
	// 黒テクスチャ作成
	void CreateBlackTexture();
	ID3D12Resource* blackTexBuff;
	// トゥーンがなかった時に使用するテクスチャ作成
	void CreateGraduationTextureBuffer();
	ID3D12Resource* gradTexBuff;				// グラデーションテクスチャ
	// トゥーンテクスチャ作成
	void CreateToonTexture();
	std::vector<ID3D12Resource*> toonBuff;		// トゥーンテクスチャ
	// インデックスを元にトゥーンのパスをもらう
	std::string GetToonPathFromIndex(const std::string& folder, int idx);

	// string(マルチバイト文字列)からwstring(ワイド文字列)を得る
	//@param str マルチバイト文字列
	//@return 変換されたワイド文字列
	std::wstring GetWideStringFromString(std::string& str);

public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

