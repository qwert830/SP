#pragma once
#define COUNT 100
#include"d3dUtil.h"
#include"GameTimer.h"
using namespace DirectX;

class Particle
{
private:
	float mTime[COUNT];
	float mIsDraw[COUNT];
	XMFLOAT3 mStartPosition;
	XMFLOAT3 mCurrentPosition[COUNT];
	XMFLOAT3 mMoveSpeed[COUNT];  
	
	bool mIsDrawParticle = false;

public:
	Particle();
	~Particle();

	void SetPosition(XMFLOAT3 pos);
	void SetMoveSpeed(XMFLOAT3 pos, XMFLOAT3 cameraPos);
	void SetDraw();
	void SetStartPaticle(XMFLOAT3 pos, XMFLOAT3 cameraPos);
	void Update(const GameTimer& gt);
	XMFLOAT3 GetCurrentPosition(int index);
	XMFLOAT3 GetMoveSpeed(int index);
	bool isDraw();
	float GetIsDraw(int index);
};

