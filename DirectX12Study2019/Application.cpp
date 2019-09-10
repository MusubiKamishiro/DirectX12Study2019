#include <iostream>
#include "Application.h"

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600


Application& Application::Instance()
{
	static Application instance;
	return instance;
};

// ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)	// ｳｨﾝﾄﾞｳが破棄されると呼ばれる
	{
		PostQuitMessage(0);	// OSに対し「このｱﾌﾟﾘを終了する」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	// 規定の処理を行う
}

Application::Application()
{
}


void Application::InitWindow()
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// コールバック関数の指定
	w.lpszClassName = "DirectX12Study2019";		// アプリケーションクラス名
	w.hInstance = GetModuleHandle(0);			// ハンドルの取得
	RegisterClassEx(&w);	// アプリケーションクラス

	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };	// ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);	// ウィンドウのサイズを補正する

	hwnd = CreateWindow(w.lpszClassName,	// クラス名指定
		"2度目のDirectX12",		// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,			// 表示X座標はOSにお任せ
		CW_USEDEFAULT,			// 表示Y座標はOSにお任せ
		wrc.right - wrc.left,	// ウィンドウ幅
		wrc.bottom - wrc.top,	// ウィンドウ高さ
		nullptr,		// 親ウィンドウハンドル
		nullptr,		// メニューハンドル
		w.hInstance,	// 呼び出しアプリケーションハンドル
		nullptr);		// 追加パラメータ

	if (hwnd == nullptr)
	{
		LPVOID messageBuffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer,
			0,
			nullptr);
		OutputDebugString((TCHAR*)messageBuffer);
		std::cout << (TCHAR*)messageBuffer << std::endl;
		LocalFree(messageBuffer);
	}
}

Application::~Application()
{
}

bool Application::Initialize()
{
	InitWindow();

	return true;
}

void Application::Run()
{
	ShowWindow(hwnd, SW_SHOW);
	MSG msg = {};
	while (true)
	{
		// OSからのメッセージをmsgに格納
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);	// 仮想キー関連の変換
			DispatchMessage(&msg);	// 処理されなかったメッセージをOSに投げ返す
		}

		// もうアプリケーションが終わってるときにWM_QUITになる
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
}

void Application::Terminate()
{
	UnregisterClass(w.lpszClassName, w.hInstance);	// 使用しなくなるため登録解除
}
