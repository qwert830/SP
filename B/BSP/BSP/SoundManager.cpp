#include "SoundManager.h"



SoundManager::SoundManager()
{
}


SoundManager::~SoundManager()
{
}

void SoundManager::Init()
{
	mResult = System_Create(&mSystem);

	mResult = mSystem->init(32, FMOD_INIT_NORMAL, NULL);
	//mResult = mSystem->createSound

}
