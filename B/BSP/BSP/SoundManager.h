#pragma once
#include <fmod.hpp>
#include <iostream>
#include "d3dUtil.h"
#include "GameTimer.h"
#pragma comment(lib, "fmod64_vc.lib")
using namespace FMOD;

enum soundName
{
	FIRESOUND = 0
};

class SoundManager
{
private:
	System *mSystem;
	Sound  *mSound[10];
	Channel *mChannel[100];
	Channel *mBSound;
	FMOD_RESULT mResult;
	
	int channelCount = 0;

public:
	SoundManager();
	~SoundManager();

	void Init();
	void SetSound(int index, XMFLOAT3 pos, XMFLOAT3 soundPos);
};

