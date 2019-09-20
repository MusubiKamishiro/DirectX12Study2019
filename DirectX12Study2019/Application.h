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
	Application(const Application&);	// ��߰�֎~
	void operator=(const Application&);	// ����֎~

	bool InitWindow();	// �E�B���h�E�̏�����

	HWND hwnd;
	WNDCLASSEX w = {};

	std::shared_ptr<Dx12Wrapper> dx12Wrapper;

	Size windowSize;

public:
	static Application& Instance();
	~Application();

	bool Initialize();	// ������, �N�����Ɉ�񂾂��Ă�
	void Run();			// ���C�����[�v, ���t���[���Ă�
	void Terminate();	// �㏈��, �I�����Ɉ�񂾂��Ă�

	Size GetWindowSize();		// ��ʃT�C�Y�̎擾
};

