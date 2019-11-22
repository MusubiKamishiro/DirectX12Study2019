#pragma once
#include <DirectXMath.h>

#include <vector>
#include <map>
#include <string>


// モーションデータ
struct VMDMotionData
{
	char boneName[15];					// ボーン名
	unsigned int frameNo;				// フレーム番号(読込時は現在のフレーム位置を0とした相対位置)
	DirectX::XMFLOAT3 location;			// 位置(IKの時に使用予定)
	DirectX::XMFLOAT4 rotation;			// Quaternion	// 回転
	unsigned char interpolation[64];	// [4][4][4]	// 補完
};

// 表情データ
struct VMDSkinData
{
	char skinName[15];		// 表情名
	unsigned int frameNo;	// フレーム番号
	float weight;			// 表情の設定値(表情スライダーの値)
};

// カメラデータ
struct VMDCameraData
{
	unsigned int frameNo;		// フレーム番号
	float length;				// -(距離)		// おそらくカメラ中心からの距離	※この値が0の場合はlocationがそのままカメラの座標になる
	DirectX::XMFLOAT3 location;	// 位置			// カメラの注視点
	DirectX::XMFLOAT3 rotation;	// オイラー角	// X軸は符号が反転しているので注意 // 回転
	char interpolation[24];		// おそらく[6][4](未検証) // 補完
	unsigned int viewIngAngle;	// 視界角
	char perspective;			// 0:on 1:off	// パースペクティブ
};

// 照明データ
struct VMDLightData
{
	unsigned int frameNo;		// フレーム番号
	DirectX::XMFLOAT3 color;	// RGB各値/256
	DirectX::XMFLOAT3 location;	// X, Y, Z
};

// セルフシャドウデータ
struct VMDSelfShadowData
{
	unsigned int frameNo;	// フレーム番号
	char mode;				// 00-02 // モード
	float distance;			// 0.1 - (dist * 0.00001) // 距離
};

// IKデータ用構造体
struct VMDIKData
{
	char ikBoneName[20];		// IKボーン名
	unsigned char ikEnabled;	// IK有効	// 0:off, 1:on
};

// 表示・IKデータ用構造
struct VMDVisibleIKData
{
	int frameNo = 0;				// フレーム番号
	unsigned char visible = 0;		// 表示	// 0:off, 1:on
	int ikCount = 0;				// IK数
	std::vector<VMDIKData> ikDatas;	// IKデータリスト
};

// フレームの位置と回転情報
struct BoneKeyFrames
{
	BoneKeyFrames() : frameNo(0), pos(0, 0, 0), quaternion(0, 0, 0, 0) {};
	BoneKeyFrames(int f, DirectX::XMFLOAT3 p, DirectX::XMFLOAT4 q) : frameNo(f), pos(p), quaternion(q) {};
	int frameNo;					// フレーム番号
	DirectX::XMFLOAT3 pos;			// 座標
	DirectX::XMFLOAT4 quaternion;	// 回転情報
};

// フレームの位置と表情スライダーの値
struct SkinKeyFrames
{
	SkinKeyFrames() : frameNo(0), weight(0.0f) {};
	SkinKeyFrames(int f, float w) : frameNo(f), weight(w) {};
	int frameNo;
	float weight;
};

class VMDLoader
{
private:
	// vmdファイルの読み込み
	void Load(const std::string& filepath);
	std::vector<VMDMotionData> motiondatas;
	std::vector<VMDSkinData> skindatas;
	std::vector<VMDCameraData> cameradatas;
	std::vector<VMDLightData> lightdatas;
	std::vector<VMDSelfShadowData> selfshadowdatas;
	std::vector<VMDVisibleIKData> visibleIKDatas;

	// アニメーションデータの初期化
	void InitAnimationData();
	std::map<std::string, std::vector<BoneKeyFrames>> animationData;	// <ボーン名, フレーム位置と回転情報>

	// 表情データの初期化
	void InitSkinData();
	std::map<std::string, std::vector<SkinKeyFrames>> skinData;

	// カメラデータをフレーム順にソート
	void SortCameraData();

	// モーションの時間を求める
	void SearchMaxFrame();
	int maxFrame;

public:
	VMDLoader(const std::string& filepath);
	~VMDLoader();

	const std::map<std::string, std::vector<BoneKeyFrames>>& GetAnimationData()const;
	const std::map<std::string, std::vector<SkinKeyFrames>>& GetSkinData()const;
	const int GetMaxFrame()const;
	const std::vector<VMDCameraData>& GetCameraData()const;
};

