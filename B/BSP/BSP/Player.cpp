#include "Player.h"
#include <WindowsX.h>
Player::Player()
{
	mPlayerID = -1;
	mSensitivity = 0.25f;
	mCameraOffsetY = 20;
	offset = { 2.0f, 18.5, 2.0f };
	ZeroMemory(mVector, sizeof(mVector));
}

Player::~Player()
{
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
		if(mMoveState<=RIGHTUP)
			mMoveState = STAND;
	}

	if ((GetAsyncKeyState('S') & 0x8000) == 0)
	{
		if (mMoveState >= LEFTDOWN)
			mMoveState = STAND;
	}

	if ((GetAsyncKeyState('A') & 0x8000) == 0)
	{
		if (mMoveState <= RIGHTUP)
			mMoveState = UP;
		else if (mMoveState >= LEFTDOWN)
			mMoveState = DOWN;
		else
			mMoveState = STAND;
	}

	if ((GetAsyncKeyState('D') & 0x8000) == 0)
	{
		if (mMoveState <= RIGHTUP)
			mMoveState = UP;
		else if (mMoveState >= LEFTDOWN)
			mMoveState = DOWN;
		else
			mMoveState = STAND;
	}

	if (GetAsyncKeyState('W') & 0x8000)
	{
		mMoveState = UP;
	}
	
	if (GetAsyncKeyState('S') & 0x8000)
	{
		mMoveState = DOWN;
	}
	
	if (GetAsyncKeyState('A') & 0x8000)
	{
		if (mMoveState <= RIGHTUP)
			mMoveState = LEFTUP;
		else if (mMoveState >= LEFTDOWN)
			mMoveState = LEFTDOWN;
		else
			mMoveState = LEFT;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		if (mMoveState <= RIGHTUP)
			mMoveState = RIGHTUP;
		else if (mMoveState >= LEFTDOWN)
			mMoveState = RIGHTDOWN;
		else
			mMoveState = RIGHT;
	}
	// ------------------------------------------------------


	// test용 코드
	// --------------------------------
	if (GetAsyncKeyState('1') & 0x8000)
		SelectPlayer(1);
	if (GetAsyncKeyState('2') & 0x8000)
		SelectPlayer(2);
	if (GetAsyncKeyState('3') & 0x8000)
		SelectPlayer(3);
	if (GetAsyncKeyState('4') & 0x8000)
		SelectPlayer(4);
	// --------------------------------

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
		if (mAttackState != ATTACK::UNABLE_ATTACK)
		{
			mLButtonDown = true;
			mRButtonDown = false;
		}
	}
	if (btnState == VK_RBUTTON)
	{
		if (mAttackState != ATTACK::UNABLE_ATTACK)
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

void Player::Forward(float d)
{
	if (mPlayerID < 0)
		return;
	DirectX::XMFLOAT3 up = { 0,1,0 };
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[mPlayerID].mRight);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&up);
	DirectX::XMVECTOR l = DirectX::XMVector3Cross(r, u);
	DirectX::XMVECTOR p = XMLoadFloat3(&mVector[mPlayerID].mPosition);
	DirectX::XMStoreFloat3(&mVector[mPlayerID].mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
}

void Player::Strafe(float d)
{
	if (mPlayerID < 0)
		return;
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[mPlayerID].mRight);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&mVector[mPlayerID].mPosition);
	DirectX::XMStoreFloat3(&mVector[mPlayerID].mPosition, DirectX::XMVectorMultiplyAdd(s, r, p));
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
	AttackUpdate(dt);
	//MoveUpdate(dt); 
	mCamera.SetPosition(mVector[mPlayerID].mPosition.x, mVector[mPlayerID].mPosition.y + mCameraOffsetY, mVector[mPlayerID].mPosition.z);
	mCamera.UpdateViewMatrix();

	//mCurrentHP -= 10.0*dt;
}

void Player::AttackUpdate(const float & dt)
{
	for (int i = 0; i < 10; ++i)
		mAttackCools[i] += dt;

	if (mAttackState != ATTACK::UNABLE_ATTACK)
	{
		if (mLButtonDown)
			mAttackState = ATTACK::GUN;
		else if (mRButtonDown)
			mAttackState = ATTACK::LASER;
	}
	
	switch (mAttackState)
	{
	case ATTACK::GUN:
		if (mAttackCools[0] < ATTACK_DELAY)
			break;
		mSuperheat += 5;
		RotateY(mCamera.ShakeCamera());
		mAttackState = ATTACK::NOATTACK;
		mAttackCools[0] = 0;
		mAttack = true;
		break;
	case ATTACK::LASER:
		mSuperheat += 150 * dt;
		mAttackState = ATTACK::NOATTACK;
		break;
	case ATTACK::UNABLE_ATTACK:
		mSuperheat -= 35 * dt;
		if (mSuperheat <= 0)
		{
			mSuperheat = 0;
			mAttackState = ATTACK::NOATTACK;
		}
		break;
	}
	mSuperheat = ClampFloat(mSuperheat - 10.0f*dt, 0.0f, 100.0f);
	if (mSuperheat >= 99.9f)
	{
		mAttackState = ATTACK::UNABLE_ATTACK;
	}
}

void Player::MoveUpdate(const float & dt)
{
	switch (mMoveState)
	{
	case LEFTUP:
		Strafe(-mMoveSpeed *dt);
		Forward(mMoveSpeed*dt);
		break;
	case UP:
		Forward(mMoveSpeed*dt);
		break;
	case RIGHTUP:
		Strafe(mMoveSpeed*dt);
		Forward(mMoveSpeed*dt);
		break;
	case LEFT:
		Strafe(-mMoveSpeed *dt);
		break;
	case RIGHT:
		Strafe(mMoveSpeed*dt);
		break;
	case LEFTDOWN:
		Strafe(-mMoveSpeed *dt);
		Forward(-mMoveSpeed *dt);
		break;
	case DOWN:
		Forward(-mMoveSpeed *dt);
		break;
	case RIGHTDOWN:
		Strafe(mMoveSpeed*dt);
		Forward(-mMoveSpeed *dt);
		break;
	}
}

const char Player::GetPlayerID()
{
	return mPlayerID;
}

unsigned char Player::GetMoveState()
{
	return mMoveState;
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

float Player::IsAttack(int i)
{
	if (mAttackCools[i] < ATTACK_DELAY)
		return mAttackCools[i]*2;
	return -1.0f;
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
