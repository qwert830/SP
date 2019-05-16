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

	float mCameraOffsetY = 20;

	char mPlayerID = 0;
	float mSensitivity = 0;
	unsigned char mPlayerTeam;

	unsigned char mMoveState = MOVE::STAND;
	unsigned char mAttackState = ATTACK::NOATTACK;

	const float mMoveSpeed = 600.0f;

	float mSuperheat = 0.0f;
	float mAttackCools[10] = { 0.1f, };

	float mMaxHP = 300.0f;
	float mCurrentHP = 300.0f;

	bool mLButtonDown = false;
	bool mRButtonDown = false;

	POINT mMousePos;

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
	
	float IsAttack(int index);
	void SetTeam(unsigned char team);
	void SetHP(float hp);
	const char GetPlayerID();
	unsigned char GetMoveState();
	float GetSuperheat();
	float GetCurrentHP();
	float GetMaxHP();
	DirectX::XMFLOAT3 GetPlayerLookVector();
	DirectX::XMFLOAT3 GetPlayerRightVector();

	POINT mLastMousePos;
};

