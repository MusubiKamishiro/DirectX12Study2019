#pragma once
#include <Windows.h>

class Application
{
private:
	Application();
	Application(const Application&);	// ｺﾋﾟｰ禁止
	void operator=(const Application&);	// 代入禁止

	void InitWindow();

	HWND hwnd;
	WNDCLASSEX w = {};

public:
	static Application& Instance();
	~Application();

	bool Initialize();	// 初期化, 起動時に一回だけ呼ぶ
	void Run();			// メインループ, 毎フレーム呼ぶ
	void Terminate();	// 後処理, 終了時に一回だけ呼ぶ
};

