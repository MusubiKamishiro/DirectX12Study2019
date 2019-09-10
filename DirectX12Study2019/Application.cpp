#include <iostream>
#include "Application.h"

#define WINDOW_WIDTH	800
#define WINDOW_HEIGHT	600


Application& Application::Instance()
{
	static Application instance;
	return instance;
};

// �E�B���h�E�v���V�[�W��
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)	// ����޳���j�������ƌĂ΂��
	{
		PostQuitMessage(0);	// OS�ɑ΂��u���̱��؂��I������v�Ɠ`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	// �K��̏������s��
}

Application::Application()
{
}


void Application::InitWindow()
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;	// �R�[���o�b�N�֐��̎w��
	w.lpszClassName = "DirectX12Study2019";		// �A�v���P�[�V�����N���X��
	w.hInstance = GetModuleHandle(0);			// �n���h���̎擾
	RegisterClassEx(&w);	// �A�v���P�[�V�����N���X

	RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };	// �E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);	// �E�B���h�E�̃T�C�Y��␳����

	hwnd = CreateWindow(w.lpszClassName,	// �N���X���w��
		"2�x�ڂ�DirectX12",		// �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,			// �\��X���W��OS�ɂ��C��
		CW_USEDEFAULT,			// �\��Y���W��OS�ɂ��C��
		wrc.right - wrc.left,	// �E�B���h�E��
		wrc.bottom - wrc.top,	// �E�B���h�E����
		nullptr,		// �e�E�B���h�E�n���h��
		nullptr,		// ���j���[�n���h��
		w.hInstance,	// �Ăяo���A�v���P�[�V�����n���h��
		nullptr);		// �ǉ��p�����[�^

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
		// OS����̃��b�Z�[�W��msg�Ɋi�[
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);	// ���z�L�[�֘A�̕ϊ�
			DispatchMessage(&msg);	// ��������Ȃ��������b�Z�[�W��OS�ɓ����Ԃ�
		}

		// �����A�v���P�[�V�������I����Ă�Ƃ���WM_QUIT�ɂȂ�
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
}

void Application::Terminate()
{
	UnregisterClass(w.lpszClassName, w.hInstance);	// �g�p���Ȃ��Ȃ邽�ߓo�^����
}
