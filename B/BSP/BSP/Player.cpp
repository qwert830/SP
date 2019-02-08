#include "Player.h"
#include <WindowsX.h>
Player::Player()
{
	PlayerID = -1;
	sensitivity = 0.25f;
	ZeroMemory(mVector, sizeof(mVector));
}

Player::~Player()
{
}

void Player::SelectPlayer(const char i)
{
	PlayerID = i;
	mCamera.SetCamera(mVector[PlayerID].mPosition, mVector[PlayerID].mRight, mVector[PlayerID].mLook, mVector[PlayerID].mUp);
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

	mCamera.UpdateViewMatrix();
}

void Player::PlayerMouseMove(WPARAM btnState, int x, int y)
{
	float dx = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(x - mouse.x));
	float dy = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(y - mouse.y));

	//Pitch(dy); //캐릭터 발을 기준으로 x축 회전함 사용하지 않을 것으로 예상됨
	RotateY(dx);
	mCamera.Pitch(dy);
	mCamera.RotateY(dx);

	SetCursorPos(mouse.x,mouse.y);
}

void Player::PlayerMouseDown(WPARAM btnState, int x, int y)
{
	if (btnState == VK_LBUTTON)
	{
		superheat += 10;
		mCamera.ShakeCamera();
	}
}

void Player::SetMousePos(int x, int y)
{
	mouse.x = x;
	mouse.y = y;
}

void Player::Forward(float d)
{
	if (PlayerID < 0)
		return;
	DirectX::XMFLOAT3 up = { 0,1,0 };
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[PlayerID].mRight);
	DirectX::XMVECTOR u = DirectX::XMLoadFloat3(&up);
	DirectX::XMVECTOR l = DirectX::XMVector3Cross(r, u);
	DirectX::XMVECTOR p = XMLoadFloat3(&mVector[PlayerID].mPosition);
	DirectX::XMStoreFloat3(&mVector[PlayerID].mPosition, DirectX::XMVectorMultiplyAdd(s, l, p));
}

void Player::Strafe(float d)
{
	if (PlayerID < 0)
		return;
	DirectX::XMVECTOR s = DirectX::XMVectorReplicate(d);
	DirectX::XMVECTOR r = DirectX::XMLoadFloat3(&mVector[PlayerID].mRight);
	DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&mVector[PlayerID].mPosition);
	DirectX::XMStoreFloat3(&mVector[PlayerID].mPosition, DirectX::XMVectorMultiplyAdd(s, r, p));
}

void Player::Pitch(float angle)
{
	if (PlayerID < 0)
		return;
	
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(DirectX::XMLoadFloat3(&mVector[PlayerID].mRight), angle);

	DirectX::XMStoreFloat3(&mVector[PlayerID].mUp,		DirectX::XMVector3TransformNormal(XMLoadFloat3(&mVector[PlayerID].mUp), R));
	DirectX::XMStoreFloat3(&mVector[PlayerID].mLook,	DirectX::XMVector3TransformNormal(XMLoadFloat3(&mVector[PlayerID].mLook), R));
}

void Player::RotateY(float angle)
{
	if (PlayerID < 0)
		return;
	
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);

	DirectX::XMStoreFloat3(&mVector[PlayerID].mRight,	DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[PlayerID].mRight), R));
	DirectX::XMStoreFloat3(&mVector[PlayerID].mUp,		DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[PlayerID].mUp), R));
	DirectX::XMStoreFloat3(&mVector[PlayerID].mLook,	DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&mVector[PlayerID].mLook), R));
}

void Player::Update(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();
	superheat = ClampFloat(superheat - 1.0f*dt, 0.0f, 100.0f);
	switch (moveState)
	{
	case LEFTUP:
		Strafe(-100.0f*dt);
		Forward(100.0f*dt);
		mCamera.Strafe(-10.0f*dt);
		mCamera.Forward(10.0f*dt);
		break;
	case UP:
		Forward(100.0f*dt);
		mCamera.Forward(10.0f*dt);
		break;
	case RIGHTUP:
		Strafe(100.0f*dt);
		Forward(100.0f*dt);
		mCamera.Strafe(10.0f*dt);
		mCamera.Forward(10.0f*dt);
		break;
	case LEFT:
		Strafe(-100.0f*dt);
		mCamera.Strafe(-10.0f*dt);
		break;
	case RIGHT:
		Strafe(100.0f*dt);
		mCamera.Strafe(10.0f*dt);
		break;
	case LEFTDOWN:
		Strafe(-100.0f*dt);
		Forward(-100.0f*dt);
		mCamera.Strafe(-10.0f*dt);
		mCamera.Forward(-10.0f*dt);
		break;
	case DOWN:
		Forward(-100.0f*dt);
		mCamera.Forward(-10.0f*dt);
		break;
	case RIGHTDOWN:
		Strafe(100.0f*dt);
		Forward(-100.0f*dt);
		mCamera.Strafe(10.0f*dt);
		mCamera.Forward(-10.0f*dt);
		break;
	}

}

const char Player::GetPlayerID()
{
	return PlayerID;
}

float Player::GetSuperheat()
{
	return superheat;
}
