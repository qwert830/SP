#include "Player.h"
#include <WindowsX.h>
Player::Player()
{
	mPlayerID = -1;
	sensitivity = 0.25f;
	k = 20;
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
	// 0x0000 ������ ���� ���� ���� ȣ�� �������� �ȴ��� ����
	// 0x8000 ������ ���� ���� ���� ȣ�� �������� ���� ����
	// 0x8001 ������ ���� ���� �ְ� ȣ�� �������� ���� ����
	// 0x0001 ������ ���� ���� �ְ� ȣ�� �������� �ȴ��� ����

	const float dt = gt.DeltaTime();

	// ���� ������ ��Ŷ��� �κ����� ����
	// ------------------------------------------------------
	if ((GetAsyncKeyState('W') & 0x8000) == 0)
	{
		if(moveState<=RIGHTUP)
			moveState = STAND;
	}

	if ((GetAsyncKeyState('S') & 0x8000) == 0)
	{
		if (moveState >= LEFTDOWN)
			moveState = STAND;
	}

	if ((GetAsyncKeyState('A') & 0x8000) == 0)
	{
		if (moveState <= RIGHTUP)
			moveState = UP;
		else if (moveState >= LEFTDOWN)
			moveState = DOWN;
		else
			moveState = STAND;
	}

	if ((GetAsyncKeyState('D') & 0x8000) == 0)
	{
		if (moveState <= RIGHTUP)
			moveState = UP;
		else if (moveState >= LEFTDOWN)
			moveState = DOWN;
		else
			moveState = STAND;
	}

	if (GetAsyncKeyState('W') & 0x8000)
	{
		moveState = UP;
	}
	
	if (GetAsyncKeyState('S') & 0x8000)
	{
		moveState = DOWN;
	}
	
	if (GetAsyncKeyState('A') & 0x8000)
	{
		if (moveState <= RIGHTUP)
			moveState = LEFTUP;
		else if (moveState >= LEFTDOWN)
			moveState = LEFTDOWN;
		else
			moveState = LEFT;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		if (moveState <= RIGHTUP)
			moveState = RIGHTUP;
		else if (moveState >= LEFTDOWN)
			moveState = RIGHTDOWN;
		else
			moveState = RIGHT;
	}
	// ------------------------------------------------------


	// test�� �ڵ�
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

	mCamera.UpdateViewMatrix();
}

void Player::PlayerMouseMove(WPARAM btnState, int x, int y)
{
	float dx = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(x - mouse.x));
	float dy = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(y - mouse.y));

	//Pitch(dy); //ĳ���� ���� �������� x�� ȸ���� ������� ���� ������ �����
	RotateY(dx);
	mCamera.Pitch(dy);
	mCamera.RotateY(dx);

	SetCursorPos(mouse.x,mouse.y);
}

void Player::PlayerMouseDown(WPARAM btnState, int x, int y)
{
	if (btnState == VK_LBUTTON)
	{
		if (attackState != ATTACK::UNABLE_ATTACK)
		{
			LButtonDown = true;
			RButtonDown = false;
		}
	}
	if (btnState == VK_RBUTTON)
	{
		if (attackState != ATTACK::UNABLE_ATTACK)
		{
			LButtonDown = false;
			RButtonDown = true;
		}
	}
}

void Player::PlayerMouseUp(WPARAM btnState, int x, int y)
{
	LButtonDown = false;
	RButtonDown = false;
}

void Player::SetMousePos(int x, int y)
{
	mouse.x = x;
	mouse.y = y;
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
	MoveUpdate(dt); 
}

void Player::AttackUpdate(const float & dt)
{
	attackCool += dt;
	if (attackState != ATTACK::UNABLE_ATTACK)
	{
		if (LButtonDown)
			attackState = ATTACK::GUN;
		else if (RButtonDown)
			attackState = ATTACK::LASER;
	}
	switch (attackState)
	{
	case ATTACK::GUN:
		if (attackCool < ATTACK_DELAY)
			break;
		superheat += 5;
		RotateY(mCamera.ShakeCamera());
		attackState = ATTACK::NOATTACK;
		attackCool = 0;
		break;
	case ATTACK::LASER:
		superheat += 150 * dt;
		attackState = ATTACK::NOATTACK;
		break;
	case ATTACK::UNABLE_ATTACK:
		superheat -= 35 * dt;
		if (superheat <= 0)
		{
			superheat = 0;
			attackState = ATTACK::NOATTACK;
		}
		break;
	}
	superheat = ClampFloat(superheat - 10.0f*dt, 0.0f, 100.0f);
	if (superheat >= 99.9f)
	{
		attackState = ATTACK::UNABLE_ATTACK;
	}
}

void Player::MoveUpdate(const float & dt)
{
	switch (moveState)
	{
	case LEFTUP:
		Strafe(-moveSpeed *dt);
		Forward(moveSpeed*dt);
		break;
	case UP:
		Forward(moveSpeed*dt);
		break;
	case RIGHTUP:
		Strafe(moveSpeed*dt);
		Forward(moveSpeed*dt);
		break;
	case LEFT:
		Strafe(-moveSpeed *dt);
		break;
	case RIGHT:
		Strafe(moveSpeed*dt);
		break;
	case LEFTDOWN:
		Strafe(-moveSpeed *dt);
		Forward(-moveSpeed *dt);
		break;
	case DOWN:
		Forward(-moveSpeed *dt);
		break;
	case RIGHTDOWN:
		Strafe(moveSpeed*dt);
		Forward(-moveSpeed *dt);
		break;
	}
	mCamera.SetPosition(mVector[mPlayerID].mPosition.x, mVector[mPlayerID].mPosition.y + k, mVector[mPlayerID].mPosition.z);
}

const char Player::GetPlayerID()
{
	return mPlayerID;
}

float Player::GetSuperheat()
{
	return superheat;
}

float Player::IsAttack()
{
	if (attackCool < ATTACK_DELAY)
		return attackCool*2;
	return -1.0f;
}
