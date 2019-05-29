#pragma once
#include <fmod.hpp>
#include <iostream>

using namespace FMOD;

class SoundManager
{
private:
	System *mSystem;
	Sound  *mSound;
	Channel *mChannel;
	FMOD_RESULT mResult;
	
public:
	SoundManager();
	~SoundManager();

	void Init();
};

