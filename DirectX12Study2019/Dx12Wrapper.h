#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <vector>

// PMDのヘッダファイル
struct PMD
{
	char magic[3];			// "Pmd"
	float version;			// バージョン
	char model_name[20];	// モデルの名前
	char comment[256];		// 製作者コメント
};

// 頂点データ
struct VertexData
{
	float pos[3];					// x, y, z					// 座標
	float normal_vec[3];			// nx, ny, nz				// 法線ベクトル
	float uv[2];					// u, v						// UV座標			// MMDは頂点UV
	unsigned short bone_num[2];		// ボーン番号1、番号2		// モデル変形(頂点移動)時に影響
	unsigned char bone_weight;		// ボーン1に与える影響度	// min:0 max:100	// ボーン2への影響度は、(100 - bone_weight)
	unsigned char edge_flag;		// 0:通常、1:エッジ無効		// エッジ(輪郭)が有効の場合
};

struct Vector3
{
	float x, y, z;
};

struct Vertex {
	DirectX::XMFLOAT3 pos;	// 座標
	DirectX::XMFLOAT2 uv;	// UV座標
};

struct WVP {
	DirectX::XMMATRIX world;		// ﾜｰﾙﾄﾞ
	DirectX::XMMATRIX viewProj;		// ﾋﾞｭｰﾌﾟﾛｼﾞｪｸｼｮﾝ
	DirectX::XMMATRIX wvp;			// 合成済み
	DirectX::XMMATRIX lightVP;		// ﾗｲﾄﾋﾞｭｰﾌﾟﾛｼﾞｪｸｼｮﾝ
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

	// 深度バッファの作成
	void CreateDepthBuff();
	ID3D12Resource* depthBuff = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;

	void InitConstants();	// 定数バッファの初期化
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

	///ここから一時的な試験のための物
	///確認が終わり次第削除すること
	ID3D12Resource* texBuff = nullptr;
	ID3D12DescriptorHeap* texHeap = nullptr;
	void CreateTex();	// テクスチャ画像の作成

	// PMD関連
	void Pmd();
	std::vector<char> pmdVertexDatas;	// PMD頂点データ
	std::vector<unsigned short> pmdFaceVertices;	// PMD面頂点データ
	// 頂点バッファの作成
	void CreatePmdVertexBuffer();
	ID3D12Resource* pmdVertexBuffer = nullptr;	// PMD用頂点バッファ
	ID3D12Resource* pmdIndexBuffer = nullptr;	// PMD用インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW pmdVbView = {};	// PMD用頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW pmdIbView = {};		// PMD用インデックスバッファビュー


public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	void Update();
	void Draw();
};

