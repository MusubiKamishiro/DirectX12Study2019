#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

#include <map>
#include <vector>
#include <array>
#include <string>

#include "VMDLoader.h"

class ImageManager;


// ヘッダファイル
struct PMDHeader
{
	char magic[3];		// "Pmd"
	float version;		// バージョン
	char modelName[20];	// モデルの名前
	char comment[256];	// 製作者コメント
};

// 頂点データ
struct PMDVertexData
{
	DirectX::XMFLOAT3 pos;		// x, y, z					// 座標
	DirectX::XMFLOAT3 normalVec;// nx, ny, nz				// 法線ベクトル
	DirectX::XMFLOAT2 uv;		// u, v						// UV座標			// MMDは頂点UV
	unsigned short boneNum[2];	// ボーン番号1、番号2		// モデル変形(頂点移動)時に影響
	unsigned char boneWeight;	// ボーン1に与える影響度	// min:0 max:100	// ボーン2への影響度は、(100 - bone_weight)
	unsigned char edgeFlag;		// 0:通常、1:エッジ無効		// エッジ(輪郭)が有効の場合
	unsigned char kuuhaku[2];	// パティング用
};

// マテリアルデータ
struct PMDMaterialData
{
	DirectX::XMFLOAT3 diffuseColor;	// dr, dg, db	// 減衰色
	float alpha;					// 減衰色の不透明度
	float specularity;				// スペキュラ乗算(スペキュラの鋭さ)
	DirectX::XMFLOAT3 specularColor;// sr, sg, sb	// 光沢色
	DirectX::XMFLOAT3 mirrorColor;	// mr, mg, mb	// 環境色(ambient)
	unsigned char toonIndex;		// toon??.bmp	// トゥーン
	unsigned char edgeFlag;			// 輪郭,影
	// パディング2個が予想される...ここまでで46バイト
	unsigned int faceVertCount;		// 面頂点数
	char textureFileName[20];		// テクスチャファイル名
};

// 表情用の頂点データ
struct PMDSkinVertData
{
	unsigned int skinVertIndex = 0;	// 表情用の頂点の番号(頂点リストにある番号)
	DirectX::XMFLOAT3 skinVertPos;	// x, y, z // 表情用の頂点の座標(頂点自体の座標)
};

// 表情のデータ
struct PMDSkinData
{
	char skinName[20];			// 表情名
	unsigned int skinVertCount;	// 表情用の頂点数
	char skinType;				// 表情の種類 // 0：base、1：まゆ、2：目、3：リップ、4：その他
	std::vector<PMDSkinVertData> skinVertData;	// 表情用の頂点のデータ(16Bytes/vert)
};

// 骨のデータ
struct PMDBoneData
{
	char boneName[20];					// ボーン名
	unsigned short parentBoneIndex;		// 親ボーン番号(ない場合は0xFFFF)
	unsigned short tailPosBoneIndex;	// tail位置のボーン番号(チェーン末端の場合は0xFFFF)	// 親：子は1：多なので、主に位置決め用
	unsigned char boneType;				// ボーンの種類
	unsigned short ikParentBoneIndex;	// IKボーン番号(影響IKボーン。ない場合は0)
	DirectX::XMFLOAT3 boneHeadPos;		// ボーンのヘッドの位置		// x, y, z
};

struct BoneNode
{
	int boneIdx = 0;					// ボーン行列配列と対応
	DirectX::XMFLOAT3 startPos;			// ボーン始点(関節初期座標)
	DirectX::XMFLOAT3 endPos;			// ボーン終点(次の関節座標)
	std::vector<BoneNode*> children;	// 子供たちへのリンク
};


///PMDの読み込みと描画を行うクラス
class PMDManager
{
private:
	// PMDデータの読み込み
	void Load(const std::string& filepath);
	std::vector<PMDVertexData> vertexDatas;		// 頂点データ
	std::vector<unsigned short> faceVertices;	// 面頂点データ
	std::vector<PMDMaterialData> matDatas;		// マテリアルデータ
	std::vector<PMDSkinData> skinDatas;			// 表情データ
	std::vector<PMDBoneData> bones;				// 骨
	std::array<char[100], 10> toonTexNames;		// トゥーンテクスチャの名前, 固定
	std::vector<std::string> modelTexturesPath;	// モデルに張り付けるテクスチャのパス(中身がないときもある)

	// ビューの作成
	void CreateView();
	ID3D12Resource* vertexBuffer = nullptr;	// 頂点バッファ
	ID3D12Resource* indexBuffer = nullptr;	// インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// インデックスバッファビュー

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
	ID3D12Resource* gradTexBuff;
	// トゥーンテクスチャ作成
	void CreateToonTexture(const std::string& filepath);
	std::vector<ID3D12Resource*> toonBuff;

	// マテリアルの初期化
	void InitMaterials();
	ID3D12DescriptorHeap* matDescriptorHeap;	// マテリアル用デスクリプタヒープ
	std::vector<ID3D12Resource*> materialBuffs;	// マテリアル用バッファ(マテリアル1つにつき1個)

	// 骨作成
	void CreateBone();
	DirectX::XMMATRIX* matMap = nullptr;
	ID3D12Resource* boneBuff;		// ボーン用バッファ
	ID3D12DescriptorHeap* boneHeap;	// ボーン用ヒープ
	std::vector<DirectX::XMMATRIX> boneMatrices;	// ボーン行列転送用
	std::map<std::string, BoneNode> boneMap;		// ボーンマップ

	// 表情作成
	void CreateSkin();
	std::map<std::string, std::vector<PMDSkinVertData>> skinMap;

	std::shared_ptr<ImageManager> imageManager;

	// モデルのテクスチャのパスを獲得
	std::string GetModelTexturePath(const std::string& modelpath, const char* texpath);

	// 拡張子を取得する
	//@param path 対象パスの文字列
	//@return 拡張子
	std::string GetExtension(const char* path);

	//テクスチャのパスをセパレータ文字で分離する
	//@param path 対象のパス文字列
	//@param splitter 区切り文字
	//@return 分離前後の文字列ペア
	std::pair<std::string, std::string> SplitFileName(const std::string& path, const char splitter = '*');

	// string(マルチバイト文字列)からwstring(ワイド文字列)を得る
	//@param str マルチバイト文字列
	//@return 変換されたワイド文字列
	std::wstring GetWideStringFromString(std::string& str);

	// テクスチャリソースの作成
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
		const unsigned int height = 4, const unsigned int arraySize = 1);

	// インデックスを元にトゥーンのパスをもらう
	std::string GetToonPathFromIndex(const std::string& folder, int idx);

	// 骨の回転を子まで伝える
	void RecursiveMatrixMultply(BoneNode& node, DirectX::XMMATRIX& inMat);

	// 骨を回転させる
	//@param quaternion
	void RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& quaternion);
	void RotateBone(const std::string& bonename, const DirectX::XMFLOAT4& q, const DirectX::XMFLOAT4& q2, float t = 0.0f);

	void MotionUpdate(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, const int& frame);

	void ChangeSkin(const std::string& skinname);
	void SkinUpdate(const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& frame);

	bool flag = true;
public:
	PMDManager(const std::string& filepath);
	~PMDManager();

	void Update(const std::map<std::string, std::vector<BoneKeyFrames>>& animationdata, const std::map<std::string, std::vector<SkinKeyFrames>>& skindata, const int& nowframe);
	void Draw(ID3D12GraphicsCommandList* cmdList);
};

