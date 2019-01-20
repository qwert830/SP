#include "Player.h"
#include <WindowsX.h>
Player::Player()
{
}


Player::~Player()
{
}

void Player::PlayerKeyBoardInput(const GameTimer & gt)
{
	const float dt = gt.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Forward(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Forward(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f*dt);

	if (GetAsyncKeyState(VK_F1) & 0x8000)
		sensitivity = ClampFloat(sensitivity - 0.01f, 0.01f, 1.0f);

	if (GetAsyncKeyState(VK_F2) & 0x8000)
		sensitivity = ClampFloat(sensitivity + 0.01f, 0.01f, 1.0f);

	mCamera.UpdateViewMatrix();
}

void Player::PlayerMouseMove(WPARAM btnState, int x, int y)
{
	float dx = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(x - mouse.x));
	float dy = DirectX::XMConvertToRadians(sensitivity*static_cast<float>(y - mouse.y));

	mCamera.Pitch(dy);
	mCamera.RotateY(dx);

	SetCursorPos(mouse.x,mouse.y);
}

void Player::PlayerMouseDown(WPARAM btnState, int x, int y)
{
}

void Player::SetMousePos(int x, int y)
{
	mouse.x = x;
	mouse.y = y;
}
