#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

#include <vector>
#include <array>
//#include <map>
#include <string>

// PMDのヘッダファイル
struct PMD
{
	char magic[3];		// "Pmd"
	float version;		// バージョン
	char modelName[20];	// モデルの名前
	char comment[256];	// 製作者コメント
};

// PMDの頂点データ
struct PMDVertexData
{
	float pos[3];				// x, y, z					// 座標
	float normalVec[3];			// nx, ny, nz				// 法線ベクトル
	float uv[2];				// u, v						// UV座標			// MMDは頂点UV
	unsigned short boneNum[2];	// ボーン番号1、番号2		// モデル変形(頂点移動)時に影響
	unsigned char boneWeight;	// ボーン1に与える影響度	// min:0 max:100	// ボーン2への影響度は、(100 - bone_weight)
	unsigned char edgeFlag;		// 0:通常、1:エッジ無効		// エッジ(輪郭)が有効の場合
};

// PMDのマテリアルデータ
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

// PMDの骨のデータ
struct PMDBoneData
{
	char boneName[20];					// ボーン名
	unsigned short parentBoneIndex;		// 親ボーン番号(ない場合は0xFFFF)
	unsigned short tailPosBoneIndex;	// tail位置のボーン番号(チェーン末端の場合は0xFFFF)	// 親：子は1：多なので、主に位置決め用
	unsigned char boneType;				// ボーンの種類
	unsigned short ikParentBoneIndex;	// IKボーン番号(影響IKボーン。ない場合は0)
	float boneHeadPos[3];				// ボーンのヘッドの位置		// x, y, z
};


///PMDの読み込みを行うクラス
class PMDLoader
{
private:
	ID3D12Device* device;

	std::vector<char> vertexDatas;				// 頂点データ
	std::vector<unsigned short> faceVertices;	// 面頂点データ
	std::vector<PMDMaterialData> matDatas;		// マテリアルデータ
	std::vector<PMDBoneData> bones;				// 骨
	std::array<char[100], 10> toonTexNames;		// トゥーンテクスチャの名前, 固定
	std::vector<std::string> modelTexturesPath;	// モデルに張り付けるテクスチャのパス(中身がないときもある)

	ID3D12Resource* vertexBuffer = nullptr;	// 頂点バッファ
	ID3D12Resource* indexBuffer = nullptr;	// インデックスバッファ
	D3D12_VERTEX_BUFFER_VIEW vbView = {};	// 頂点バッファビュー
	D3D12_INDEX_BUFFER_VIEW ibView = {};	// インデックスバッファビュー

	// モデルのテクスチャのパスを獲得
	std::string GetModelTexturePath(const std::string& modelpath, const char* texpath);

	// ビューの作成
	void CreateView();

public:
	PMDLoader(const std::string& filepath, ID3D12Device* dev);
	~PMDLoader();

	const std::vector<char>& GetVertexDatas()const;
	const std::vector<unsigned short>& GetFaceVertices();
	const std::vector<PMDMaterialData>& GetMatDatas();
	const std::vector<PMDBoneData>& GetBones();
	const std::array<char[100], 10>& GetToonTexNames();
	const std::vector<std::string>& GetModelTexturesPath();

	const D3D12_VERTEX_BUFFER_VIEW& GetVbView()const;
	const D3D12_INDEX_BUFFER_VIEW& GetIbView()const;
};

