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

// フレームの位置と回転情報
struct KeyFlames
{
	KeyFlames() : frameNo(0), quaternion(0, 0, 0, 0) {};
	KeyFlames(int f, DirectX::XMFLOAT4 q) : frameNo(f), quaternion(q) {};
	int frameNo;					// フレーム番号
	DirectX::XMFLOAT4 quaternion;	// 回転情報
};


class VMDLoader
{
private:
	// vmdファイルの読み込み
	void Load(const std::string& filepath);
	std::vector<VMDMotionData> motiondata;

	// アニメーションデータの初期化
	void InitAnimationData();
	// <ボーン名, フレーム位置と回転情報>
	std::map<std::string, std::vector<KeyFlames>> animationData;

	// モーションの時間を求める
	void SearchMaxFrame();
	int maxFrame;

public:
	VMDLoader(const std::string& filepath);
	~VMDLoader();

	const std::map<std::string, std::vector<KeyFlames>>& GetAnimationData()const;
	const int GetMaxFrame()const;
};

