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

	if (GetAsyncKeyState('W') & 0x8000)
	{
		Foword(10.0f*dt);
		mCamera.Forward(10.0f*dt);
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		Foword(-10.0f*dt);
		mCamera.Forward(-10.0f*dt);
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		Strafe(-10.0f*dt);
		mCamera.Strafe(-10.0f*dt);
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		Strafe(10.0f*dt);
		mCamera.Strafe(10.0f*dt);
	}

	if (GetAsyncKeyState(VK_F1) & 0x8000)
		sensitivity = ClampFloat(sensitivity - 0.01f, 0.01f, 1.0f);

	if (GetAsyncKeyState(VK_F3) & 0x8000)
		sensitivity = ClampFloat(sensitivity + 0.01f, 0.01f, 1.0f);

	if (GetAsyncKeyState('1') & 0x8000)
		SelectPlayer(1);

	mCamera.UpdateViewMatrix();
}

void Player::PlayerMouseMove(WPARAM btnState, int x, int y)
{
	float dx = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(x - mouse.x));
	float dy = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(y - mouse.y));

	Pitch(dy);
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

void Player::Foword(float d)
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
	
}



const char Player::GetPlayerID()
{
	return PlayerID;
}

float Player::GetSuperheat()
{
	return superheat;
}
