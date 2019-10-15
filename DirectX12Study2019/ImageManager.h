#pragma once
#include <d3d12.h>

#include <map>
#include <string>

class ImageManager
{
private:
	ID3D12Device* device;
	std::map<std::string, int> table;

	// �g���q���擾����
	//@param path �Ώۃp�X�̕�����
	//@return �g���q
	std::string GetExtension(const char* path);

	// string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	//@param str �}���`�o�C�g������
	//@return �ϊ����ꂽ���C�h������
	std::wstring GetWideStringFromString(const std::string& str);

	// �e�N�X�`�����\�[�X�̍쐬
	ID3D12Resource* CreateTextureResource(ID3D12Resource* buff, const unsigned int width = 4,
		const unsigned int height = 4, const unsigned int arraySize = 1);

public:
	ImageManager(ID3D12Device* dev);
	~ImageManager();

	ID3D12Resource* Load(const std::string& filepath);
};

