#pragma once
#include <fmod.hpp>
#include <iostream>
#pragma comment(lib, "fmod64_vc.lib")
using namespace FMOD;

class SoundManager
{
private:
	System *mSystem;
	Sound  *mSound[10];
	Channel *mChannel;
	FMOD_RESULT mResult;
	
public:
	SoundManager();
	~SoundManager();

	void Init();
};

