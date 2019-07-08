#include "Player.h"
#include <WindowsX.h>
Player::Player()
{
	mPlayerID = -1;
	mSensitivity = 0.25f;
	mCameraOffsetY = 20;
	offset = { 2.0f, 18.5, 2.0f };
	
	fill_n(mMoveState, 10, MOVE::STAND);
	fill_n(mAttackState, 10, ATTACK::NOATTACK);
	fill_n(mSurvival, 10, true);

	ZeroMemory(mVector, sizeof(mVector));

	mPxMod = new PhysXModule;
}

Player::~Player()
{
	delete mPxMod;
}

void Player::SelectPlayer(const int i)
{
	mPlayerID = i;
	mCamera.SetCamera(mVector[mPlayerID].mPosition, mVector[mPlayerID].mRight, mVector[mPlayerID].mLook, mVector[mPlayerID].mUp);
}

void Player::PlayerKeyBoardInput(const GameTimer & gt)
{
	// 0x0000 이전에 누른 적이 없고 호출 시점에서 안눌린 상태
	// 0x8000 이전에 누른 적이 없고 호출 시점에서 눌린 상태
	// 0x8001 이전에 누른 적이 있고 호출 시점에서 눌린 상태
	// 0x0001 이전에 누른 적이 있고 호출 시점에서 안눌린 상태

	const float dt = gt.DeltaTime();

	// 서버 연동시 패킷통신 부분으로 변경
	// ------------------------------------------------------
	if ((GetAsyncKeyState('W') & 0x8000) == 0)
	{
		if(mMoveState[0]<=RIGHTUP)
			mMoveState[0] = STAND;
	}

	if ((GetAsyncKeyState('S') & 0x8000) == 0)
	{
		if (mMoveState[0] >= LEFTDOWN)
			mMoveState[0] = STAND;
	}

	if ((GetAsyncKeyState('A') & 0x8000) == 0)
	{
		if (mMoveState[0] <= RIGHTUP)
			mMoveState[0] = UP;
		else if (mMoveState[0] >= LEFTDOWN)
			mMoveState[0] = DOWN;
		else
			mMoveState[0] = STAND;
	}

	if ((GetAsyncKeyState('D') & 0x8000) == 0)
	{
		if (mMoveState[0] <= RIGHTUP)
			mMoveState[0] = UP;
		else if (mMoveState[0] >= LEFTDOWN)
			mMoveState[0] = DOWN;
		else
			mMoveState[0] = STAND;
	}

	if (GetAsyncKeyState('W') & 0x8000)
	{
		mMoveState[0] = UP;
	}
	
	if (GetAsyncKeyState('S') & 0x8000)
	{
		mMoveState[0] = DOWN;
	}
	
	if (GetAsyncKeyState('A') & 0x8000)
	{
		if (mMoveState[0] <= RIGHTUP)
			mMoveState[0] = LEFTUP;
		else if (mMoveState[0] >= LEFTDOWN)
			mMoveState[0] = LEFTDOWN;
		else
			mMoveState[0] = LEFT;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		if (mMoveState[0] <= RIGHTUP)
			mMoveState[0] = RIGHTUP;
		else if (mMoveState[0] >= LEFTDOWN)
			mMoveState[0] = RIGHTDOWN;
		else
			mMoveState[0] = RIGHT;
	}
	// ------------------------------------------------------

	if (GetAsyncKeyState('1') & 0x8000)
		mSensitivity = 0.1f;
	if (GetAsyncKeyState('2') & 0x8000)
		mSensitivity = 0.25f;
	if (GetAsyncKeyState('3') & 0x8000)
		mSensitivity = 0.4f;
	if (GetAsyncKeyState('4') & 0x8000)
		mSensitivity = 0.55f;
}

void Player::PlayerMouseMove(WPARAM btnState, int x, int y)
{
	float dx = DirectX::XMConvertToRadians(mSensitivity*static_cast<float>(x - mMousePos.x));
	float dy = DirectX::XMConvertToRadians(mSensitivity*static_cast<float>(y - mMousePos.y));

	//Pitch(dy); //캐릭터 발을 기준으로 x축 회전함 사용하지 않을 것으로 예상됨
	RotateY(dx);
	mCamera.Pitch(dy);
	mCamera.RotateY(dx);

	SetCursorPos(mMousePos.x,mMousePos.y);
}

void Player::PlayerMouseDown(WPARAM btnState, int x, int y)
{
	if (btnState == VK_LBUTTON)
	{
		if (mAttackState[0] != ATTACK::UNABLE_ATTACK)
		{
			mLButtonDown = true;
			mRButtonDown = false;
		}
	}
	if (btnState == VK_RBUTTON)
	{
		if (mAttackState[0] != ATTACK::UNABLE_ATTACK)
		{
			mLButtonDown = false;
			mRButtonDown = true;
		}
	}
}

void Player::PlayerMouseUp(WPARAM btnState, int x, int y)
{
	mLButtonDown = false;
	mRButtonDown = false;
}

void Player::SetMousePos(int x, int y)
{
	mMousePos.x = x;
	mMousePos.y = y;
}

void Player::Forward(float d, int index)
{
	DirectX::XMFLOAT3 up = { 0,1,0 };
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[index].mRight);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&up);
	DirectX::XMVECTOR l = DirectX::XMVector3Cross(r, u);
	DirectX::XMVECTOR p = XMLoadFloat3(&mVector[index].mPosition);
	DirectX::XMStoreFloat3(&mVector[index].mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
}

void Player::Strafe(float d, int index)
{
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[index].mRight);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&mVector[index].mPosition);
	DirectX::XMStoreFloat3(&mVector[index].mPosition, DirectX::XMVectorMultiplyAdd(s, r, p));
}

void Player::Pitch(float angle)
{
	if (mPlayerID < 0)
		return;
	
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&mVector[mPlayerID].mRight), angle);

	DirectX::XMStoreFloat3(&mVector[mPlayerID].mUp,		DirectX::XMVector3TransformNormal(XMLoadFloat3(&mVector[mPlayerID].mUp), R));
	DirectX::XMStoreFloat3(&mVector[mPlayerID].mLook,	DirectX::XMVector3TransformNormal(XMLoadFloat3(&mVector[mPlayerID].mLook), R));
}

void Player::RotateY(float angle)
{
	if (mPlayerID < 0)
		return;

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);

	DirectX::XMStoreFloat3(&mVector[mPlayerID].mRight,	DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[mPlayerID].mRight), R));
	DirectX::XMStoreFloat3(&mVector[mPlayerID].mUp,		DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[mPlayerID].mUp), R));
	DirectX::XMStoreFloat3(&mVector[mPlayerID].mLook,	DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[mPlayerID].mLook), R));
}

void Player::Update(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();
	for (int i = 0; i < 10; ++i)
		mAttackCools[i] += dt;

	if (mSurvival[0])
	{
		AttackUpdate(dt);
		mCamera.SetPosition(mVector[mPlayerID].mPosition.x, mVector[mPlayerID].mPosition.y + mCameraOffsetY, mVector[mPlayerID].mPosition.z);
		mCamera.UpdateViewMatrix();

		mHit -= 2 * dt;

		mHit = max(mHit, 0);
	}
	for (int i = 0; i < 10; ++i)
	{
		if (!mSurvival[i]) continue;
		MoveUpdate(dt, i);
	}
	mPxMod->stepPhysics(gt.DeltaTime());

	if (startchk) {
		for (int i = 0; i < 10; ++i) {
			mVector[i].mPosition.x = mCapsuleController[i]->getPosition().x;
			mVector[i].mPosition.y = mCapsuleController[i]->getPosition().y - 12;
			mVector[i].mPosition.z = mCapsuleController[i]->getPosition().z;
		}
	}

}

void Player::AttackUpdate(const float & dt)
{
	if (mAttackState[0] != ATTACK::UNABLE_ATTACK)
	{
		if (mLButtonDown)
			mAttackState[0] = ATTACK::GUN;
		else if (mRButtonDown)
			mAttackState[0] = ATTACK::LASER;
	}
	
	switch (mAttackState[0])
	{
	case ATTACK::GUN:
		if (mAttackCools[0] < ATTACK_DELAY)
			break;
		mSuperheat += 5;
		RotateY(mCamera.ShakeCamera());
		mAttackState[0] = ATTACK::NOATTACK;
		mAttackCools[0] = 0;
		mAttack = true;
		break;
	case ATTACK::LASER:
		mSuperheat += 150 * dt;
		mAttackState[0] = ATTACK::NOATTACK;
		break;
	case ATTACK::UNABLE_ATTACK:
		mSuperheat -= 35 * dt;
		if (mSuperheat <= 0)
		{
			mSuperheat = 0;
			mAttackState[0] = ATTACK::NOATTACK;
		}
		break;
	}
	mSuperheat = ClampFloat(mSuperheat - 10.0f*dt, 0.0f, 100.0f);
	if (mSuperheat >= 99.9f)
	{
		mAttackState[0] = ATTACK::UNABLE_ATTACK;
	}
}

void Player::MoveUpdate(const float & dt, int i)
{
	PxVec3 spdL = PxVec3(mVector[i].mLook.x * mMoveSpeed * dt, mVector[i].mLook.y * mMoveSpeed * dt, mVector[i].mLook.z * mMoveSpeed * dt);
	PxVec3 spdR = PxVec3(mVector[i].mRight.x * mMoveSpeed * dt, mVector[i].mRight.y * mMoveSpeed * dt, mVector[i].mRight.z * mMoveSpeed * dt);
	PxControllerFilters filter;
	switch (mMoveState[i])
	{
	case LEFTUP:
		//Strafe(-mMoveSpeed * dt, i);
		//Forward(mMoveSpeed*dt, i);
		mCapsuleController[i]->move(PxVec3(spdL.x - spdR.x, 0, spdL.z - spdR.z), 0.001f, dt, filter);
		break;
	case UP:
		//Forward(mMoveSpeed*dt, i);
		mCapsuleController[i]->move(PxVec3(spdL.x, 0, spdL.z), 0.001f, dt, filter);
		break;
	case RIGHTUP:
		//Strafe(mMoveSpeed*dt, i);
		//Forward(mMoveSpeed*dt, i);
		mCapsuleController[i]->move(PxVec3(spdL.x + spdR.x, 0, spdL.z + spdR.z), 0.001f, dt, filter);
		break;
	case LEFT:
		//Strafe(-mMoveSpeed * dt, i);
		mCapsuleController[i]->move(PxVec3(-spdR.x, 0, -spdR.z), 0.001f, dt, filter);
		break;
	case RIGHT:
		//Strafe(mMoveSpeed*dt, i);
		mCapsuleController[i]->move(PxVec3(spdR.x, 0, spdR.z), 0.001f, dt, filter);
		break;
	case LEFTDOWN:
		//Strafe(-mMoveSpeed*0.5f * dt, i);
		//Forward(-mMoveSpeed * 0.5f * dt, i);
		mCapsuleController[i]->move(PxVec3(-spdL.x - spdR.x, 0, -spdL.z - spdR.z), 0.001f, dt, filter);
		break;
	case DOWN:
		//Forward(-mMoveSpeed * 0.5f * dt, i);
		mCapsuleController[i]->move(PxVec3(-spdL.x, 0, -spdL.z), 0.001f, dt, filter);
		break;
	case RIGHTDOWN:
		//Strafe(mMoveSpeed*dt, i);
		//Forward(-mMoveSpeed * 0.5f * dt, i);
		mCapsuleController[i]->move(PxVec3(spdL.x + spdR.x, 0, spdL.z + spdR.z), 0.001f, dt, filter);
		break;
	}
}

const char Player::GetPlayerID()
{
	return mPlayerID;
}

unsigned char Player::GetMoveState(int index)
{
	return mMoveState[index];
}

unsigned char Player::GetPlayerTeam()
{
	return mPlayerTeam;
}

PhysXModule* Player::GetPx()
{
	return mPxMod;
}

PxCapsuleController * Player::GetCapsCont(const int idx)
{
	return mCapsuleController[idx];
}

void Player::SetCapsCont(const int idx, PxCapsuleController * caps)
{
	mCapsuleController[idx] = caps;
}

float Player::GetSuperheat()
{
	return mSuperheat;
}

float Player::GetCurrentHP()
{
	return mCurrentHP;
}

float Player::GetMaxHP()
{
	return mMaxHP;
}

bool Player::GetAttackState()
{
	if (mAttack)
	{
		mAttack = false;
		return true;
	}
	return false;
}

bool Player::GetMoveStateDirty()
{
	if (mPreMoveState == mMoveState[0])
		return false;
	else
	{
		mPreMoveState = mMoveState[0];
		return true;
	}
}

float Player::GetHit()
{
	return mHit;
}

float Player::GetSurvival(int index)
{
	if (mSurvival[index])
	{
		return 1.0f;
	}
	else
		return -1.0f;
}

float Player::IsAttack(int i)
{
	if (mAttackCools[i] < ATTACK_DELAY)
		return mAttackCools[i]*2;
	return -1.0f;
}

void Player::SetTestMode(const bool test)
{
	mTestMode = test;
}

void Player::SetSurvival(int index, const bool survival)
{
	mSurvival[index] = survival;
}

void Player::SetMoveState(int index, unsigned char state)
{
	mMoveState[index] = state;
}

void Player::SetHit()
{
	mHit += 0.5f;
}

void Player::SetTeam(unsigned char team)
{
	mPlayerTeam = team;
	if (mPlayerTeam == RED_READER || mPlayerTeam == BLUE_READER)
	{
		mMaxHP = 300.0f;
		mCurrentHP = 300.0f;
	}
	else
	{
		mMaxHP = 100.0f;
		mCurrentHP = 100.0f;
	}
}

void Player::SetHP(float hp)
{
	mCurrentHP = hp;
}

void Player::SetAttack(int index)
{
	mAttackCools[index] = 0.0f;
}

DirectX::XMFLOAT3 Player::GetPlayerLookVector()
{
	return mVector[0].mLook;
}

DirectX::XMFLOAT3 Player::GetPlayerRightVector()
{
	return mVector[0].mRight;
}

DirectX::XMFLOAT3 Player::GetCameraPosition()
{
	return mCamera.GetPosition3f();
}

DirectX::XMFLOAT3 Player::GetCameraLookVector()
{
	return mCamera.GetLook3f();
}
