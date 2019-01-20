#pragma once
#include"d3dUtil.h"
#include"GameTimer.h"
#include"Camera.h"

enum MOVE{ LEFTUP, UP, RIGHTUP, LEFT, STAND, RIGHT, LEFTDOWN, DOWN, RIGHTDOWN };
enum ATTACK{ GUN, LASER, NOATTACK};
class Player
{
private:
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };

	unsigned int PlayerID = 0;
	float sensitivity = 0.25f;

	unsigned char moveState = MOVE::STAND;
	unsigned char attackState = ATTACK::NOATTACK;

	POINT mouse;

public:
	Player();
	~Player();

	Camera mCamera;

	void PlayerKeyBoardInput(const GameTimer& gt);
	void PlayerMouseMove(WPARAM btnState, int x, int y);
	void PlayerMouseDown(WPARAM btnState, int x, int y);
	void SetMousePos(int x, int y);
	POINT mLastMousePos;
};

