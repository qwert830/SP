#pragma once
#include"NetworkModule.h"
#include"d3dUtil.h"
#include"GameTimer.h"
#include"Camera.h"

#define ATTACK_DELAY 0.1f

enum MOVE{ LEFTUP, UP, RIGHTUP, LEFT, STAND, RIGHT, LEFTDOWN, DOWN, RIGHTDOWN };
enum ATTACK{ GUN, LASER, NOATTACK, UNABLE_ATTACK };

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

	float k = 20;

	char mPlayerID = 0;
	float sensitivity = 0;
	unsigned char mPlayerTeam;

	unsigned char moveState = MOVE::STAND;
	unsigned char attackState = ATTACK::NOATTACK;

	const float moveSpeed = 600.0f;

	float superheat = 0.0f;
	float attackCool = 0.1f;

	bool LButtonDown = false;
	bool RButtonDown = false;

	POINT mouse;

public:
	Player();
	~Player();

	Camera mCamera;
	WorldVecter mVector[10];

	DirectX::XMFLOAT3 offset;

	void SelectPlayer(const int i);
	void PlayerKeyBoardInput(const GameTimer& gt);
	void PlayerMouseMove(WPARAM btnState, int x, int y);
	void PlayerMouseDown(WPARAM btnState, int x, int y);
	void PlayerMouseUp(WPARAM btnState, int x, int y);
	void SetMousePos(int x, int y);
	void Forward(float d);
	void Strafe(float d);
	void Pitch(float angle);
	void RotateY(float angle);
	void Update(const GameTimer& gt);
	void AttackUpdate(const float& dt);
	void MoveUpdate(const float& dt);
	const char GetPlayerID();
	unsigned char GetMoveState();
	float GetSuperheat();
	float IsAttack();
	void SetTeam(unsigned char team);
	DirectX::XMFLOAT3 GetPlayerLookVector();
	DirectX::XMFLOAT3 GetPlayerRightVector();

	POINT mLastMousePos;
};

