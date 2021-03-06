#pragma once
#include "NetworkModule.h"
#include "d3dUtil.h"
#include "GameTimer.h"
#include "Camera.h"
#include "PhysXModule.h"

#define ATTACK_DELAY 0.1f

enum MOVE{ LEFTUP, UP, RIGHTUP, LEFT, STAND, RIGHT, LEFTDOWN, DOWN, RIGHTDOWN };
enum ATTACK{ GUN, LASER, NOATTACK, UNABLE_ATTACK };

struct WorldVecter
{
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
};

struct Jump
{
	bool state = false;
	bool recentstate = false;
	float recentYpos = 0;
	float recentYpos2 = 0;
	float jumpPower = 0;
};

class Player
{
private:

	float mCameraOffsetY = 20;

	char mPlayerID = 0;
	float mSensitivity = 0;
	unsigned char mPlayerTeam;

	unsigned char mPreMoveState = { MOVE::STAND };
	unsigned char mMoveState[10];
	unsigned char mAttackState[10];
	bool mSurvival[10];

	const float mMoveSpeed = 400.0f;

	float mSuperheat = 0.0f;
	float mAttackCools[10] = { 0.1f, };

	float mMaxHP = 300.0f;
	float mCurrentHP = 300.0f;
	float mHit = 0.0f;

	bool mLButtonDown = false;
	bool mRButtonDown = false;
	bool mAttack = false;

	bool mTestMode = false;

	POINT mMousePos;
	PhysXModule* mPxMod;
	PxCapsuleController* mCapsuleController[10];
	Jump mJump[10];


public:
	Player();
	~Player();

	bool startchk = false;

	Camera mCamera;
	WorldVecter mVector[10];

	DirectX::XMFLOAT3 offset;

	void SelectPlayer(const int i);
	
	void SetMousePos(int x, int y);
	void PlayerMouseUp(WPARAM btnState, int x, int y);
	void PlayerMouseMove(WPARAM btnState, int x, int y);
	void PlayerMouseDown(WPARAM btnState, int x, int y);
	void PlayerKeyBoardInput(const GameTimer& gt);

	void Pitch(float angle);
	void Strafe(float d, int index);
	void Forward(float d, int index);
	void RotateY(float angle);
	
	void Update(const GameTimer& gt);
	void AttackUpdate(const float& dt);
	void MoveUpdate(const float& dt, int i);
	
	void SetHP(float hp);
	void SetTeam(unsigned char team);
	void SetAttack(int index);
	void SetTestMode(const bool test);
	void SetSurvival(int index, const bool survival);
	void SetMoveState(int index, unsigned char state);
	void SetHit();
	void SetCapsCont(const int idx, PxCapsuleController* caps);
	void SetJumpstatus(const int idx, const Jump& setter);

	int GetAttackStateInt();
	bool GetAttackState();
	bool GetMoveStateDirty();
	float GetHit();
	float IsAttack(int index);
	float GetMaxHP();
	float GetCurrentHP();
	float GetSuperheat();
	float GetSurvival(int index);
	const char GetPlayerID();
	unsigned char GetMoveState(int index);
	unsigned char GetPlayerTeam();
	PhysXModule* GetPx();
	PxCapsuleController* GetCapsCont(const int idx);
	Jump GetJumpstatus(const int idx);


	DirectX::XMFLOAT3 GetPlayerLookVector();
	DirectX::XMFLOAT3 GetPlayerRightVector();
	DirectX::XMFLOAT3 GetCameraPosition();
	DirectX::XMFLOAT3 GetCameraLookVector();

	POINT mLastMousePos;
};

