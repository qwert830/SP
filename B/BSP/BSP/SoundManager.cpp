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
	
	mResult = mSystem->set3DSettings(1.0f, 0.1f, 1.0f);

	mResult = mSystem->createSound("Sound//flaunch.wav", FMOD_2D, 0, &mSound[0]);
	mSound[0]->setMode(FMOD_LOOP_OFF);
}

void SoundManager::SetSound(int index, XMFLOAT3 pos, XMFLOAT3 soundPos, XMFLOAT3 look)
{
	FMOD_VECTOR vel = { 0.0f, 0.0f, 0.0f };
	FMOD_VECTOR up = { 0.0f, 1.0f, 0.0f };
	
	FMOD_VECTOR posV;
	posV.x = pos.x;
	posV.y = pos.y;
	posV.z = pos.z;

	FMOD_VECTOR sp;
	sp.x = soundPos.x;
	sp.y = soundPos.y;
	sp.z = soundPos.z;

	FMOD_VECTOR l;
	l.x = look.x;
	l.y = look.y;
	l.z = look.z;

	float x = (float)fabs(pos.x - sp.x);
	float y = (float)fabs(pos.y - sp.y);
	float z = (float)fabs(pos.z - sp.z);

	float volume = 20.0f / (x + y + z);

	volume = min(max(volume, 0), 1);
	
	mSystem->playSound(mSound[index], 0, false, &mChannel[channelCount]);
	mChannel[channelCount]->set3DAttributes(&sp, &vel);
	mChannel[channelCount++]->setVolume(volume);
	
	mSystem->update();

	if (channelCount >= 100)
		channelCount = 0;
}
