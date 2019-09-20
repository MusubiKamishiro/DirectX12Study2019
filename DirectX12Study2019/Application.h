#pragma once
#include <Windows.h>
#include <memory>

class Dx12Wrapper;

struct Size
{
	Size() {};
	Size(int inw, int inh) : width(inw), height(inh) {};

	int width = 0;
	int height = 0;
};

class Application
{
private:
	Application();
	Application(const Application&);	// ｺﾋﾟｰ禁止
	void operator=(const Application&);	// 代入禁止

	bool InitWindow();	// ウィンドウの初期化

	HWND hwnd;
	WNDCLASSEX w = {};

	std::shared_ptr<Dx12Wrapper> dx12Wrapper;

	Size windowSize;

public:
	static Application& Instance();
	~Application();

	bool Initialize();	// 初期化, 起動時に一回だけ呼ぶ
	void Run();			// メインループ, 毎フレーム呼ぶ
	void Terminate();	// 後処理, 終了時に一回だけ呼ぶ

	Size GetWindowSize();		// 画面サイズの取得
};

