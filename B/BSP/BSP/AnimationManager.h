#pragma once
#include "d3dUtil.h"

class AnimationManager
{
private:
	map<string,vector<XMFLOAT4X4>> data;

public:
	AnimationManager();
	~AnimationManager();

	void Init();
	void LoadDataFromFile(const char* name, const char* aniName);
	int GetSize(const char* aniName);
	XMFLOAT4X4 GetData(const char* aniName, int index ); //추후 프레임 추가예정
};

