#pragma once
#include"d3dUtil.h"
#include"GameTimer.h"
#include"Camera.h"

enum MOVE{ LEFTUP, UP, RIGHTUP, LEFT, STAND, RIGHT, LEFTDOWN, DOWN, RIGHTDOWN };
enum ATTACK{ GUN, LASER, NOATTACK};

struct WorldVecter
{
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
};

class Player
{
private:

	char PlayerID = 0;
	float sensitivity = 0.25f;

	unsigned char moveState = MOVE::STAND;
	unsigned char attackState = ATTACK::NOATTACK;

	float superheat = 0.0f;

	POINT mouse;

public:
	Player();
	~Player();

	Camera mCamera;
	WorldVecter mVector[9];

	void SelectPlayer(const char i);
	void PlayerKeyBoardInput(const GameTimer& gt);
	void PlayerMouseMove(WPARAM btnState, int x, int y);
	void PlayerMouseDown(WPARAM btnState, int x, int y);
	void SetMousePos(int x, int y);
	void Foword(float d);
	void Strafe(float d);
	void Pitch(float angle);
	void RotateY(float angle);
	void Update(const GameTimer& gt);
	const char GetPlayerID();
	float GetSuperheat();
	POINT mLastMousePos;
};

