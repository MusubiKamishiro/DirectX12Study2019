#pragma once
#include <Windows.h>

class Application
{
private:
	Application();
	Application(const Application&);	// ��߰�֎~
	void operator=(const Application&);	// ����֎~

	void InitWindow();

	HWND hwnd;
	WNDCLASSEX w = {};

public:
	static Application& Instance();
	~Application();

	bool Initialize();	// ������, �N�����Ɉ�񂾂��Ă�
	void Run();			// ���C�����[�v, ���t���[���Ă�
	void Terminate();	// �㏈��, �I�����Ɉ�񂾂��Ă�
};

