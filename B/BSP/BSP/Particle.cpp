#include "Particle.h"



Particle::Particle()
{
}


Particle::~Particle()
{
}

void Particle::SetPosition(XMFLOAT3 pos)
{
	mStartPosition = pos;
	for (auto& d : mCurrentPosition)
		d = pos;
}

void Particle::SetMoveSpeed(XMFLOAT3 pos, XMFLOAT3 cameraPos)
{
	XMFLOAT3 look;
	XMStoreFloat3(&look, XMVector3Normalize( (XMLoadFloat3(&cameraPos) - XMLoadFloat3(&pos))));
	for (int i = 0; i < COUNT; ++i)
	{
		float dx = (float)(rand() % 1000) / 150.0f;
		float dy = (float)(rand() % 1000) / 150.0f;
		float dz = (float)(rand() % 1000) / 150.0f;
		float lx = (float)(rand() % 1000) / 1000.0f - 0.4f;
		float lz = (float)(rand() % 1000) / 1000.0f - 0.4f;
		mMoveSpeed[i].x = (look.x+lx) * dx;
		mMoveSpeed[i].y = (float)((rand() % 1001) - 500) / 500.0f * dy;
		mMoveSpeed[i].z = (look.z+lz) * dz;

		mTime[i] = 0;
	}
}

void Particle::SetDraw()
{
	for (auto& d : mIsDraw)
	{
		d = 1.0f;
	}
}

void Particle::SetStartPaticle(XMFLOAT3 pos, XMFLOAT3 cameraPos)
{
	SetPosition(pos);
	SetMoveSpeed(pos, cameraPos);
	SetDraw();
	mIsDrawParticle = true;
}

void Particle::Update(const GameTimer & gt)
{
	if (mIsDrawParticle)
	{
		int temp = 0;
		auto t = gt.DeltaTime();
		for (int i = 0; i < COUNT; ++i)
		{
			mTime[i] += (2 + 0.1f*(fabsf(mMoveSpeed[i].x) + fabsf(mMoveSpeed[i].y) + fabsf(mMoveSpeed[i].z))) * t;
			if (mTime[i] > 1)
			{
				mIsDraw[i] = -1;
				temp++;
				if (temp == COUNT)
					mIsDrawParticle = false;
				continue;
			}

			float x = mStartPosition.x + mMoveSpeed[i].x * mTime[i]*3;
			float y = mStartPosition.y + mMoveSpeed[i].y * mTime[i]*2;
			float z = mStartPosition.z + mMoveSpeed[i].z * mTime[i]*3;

			mCurrentPosition[i] = XMFLOAT3(x, y, z);
		}
	}
}

XMFLOAT3 Particle::GetCurrentPosition(int index)
{
	return mCurrentPosition[index];
}

XMFLOAT3 Particle::GetMoveSpeed(int index)
{
	return mMoveSpeed[index];
}

bool Particle::isDraw()
{
	return mIsDrawParticle;
}

float Particle::GetIsDraw(int index)
{
	return mIsDraw[index];
}
