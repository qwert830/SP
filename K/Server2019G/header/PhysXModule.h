#pragma once
#include "PxPhysicsAPI.h"
#include <string>
using namespace physx;
using namespace std;

#define PVD_HOST "127.0.0.1"

class CharacterCollisionCallback : public PxUserControllerHitReport
{
private:

public:
	void onShapeHit(const PxControllerShapeHit &hit) {

	}
	void onControllerHit(const PxControllersHit &hit) {

	}
	void onObstacleHit(const PxControllerObstacleHit &hit) {
	}
};

class PhysXModule
{
protected:
	PxDefaultAllocator		mAllocator;
	PxDefaultErrorCallback	mErrorCallback;
	PxFoundation*			mFoundation = NULL;
	PxPhysics*				mPhysics = NULL;
	PxDefaultCpuDispatcher*	mDispatcher = NULL;
	PxScene*				mScene = NULL;
	PxCooking*				mCooking;
	PxMaterial*				mMaterial = NULL;
	PxControllerManager*	mControllerManager;
	CharacterCollisionCallback collisionCallback;

#ifdef _DEBUG
	PxPvd*                  mPvd = NULL;
#endif
public:
	PhysXModule();
	~PhysXModule();

	void stepPhysics(const PxReal& frame);
	void setGravity(const PxVec3& gravityP);

	//void setCapsuleController(PxCapsuleController* mCapsuleController, PxExtendedVec3 pos, float height, float radius, PxUserControllerHitReport * collisionCallback);
	PxCapsuleController* setCapsuleController(PxExtendedVec3 pos, float height, float radius, int key);
	
	virtual pair<int, PxVec3> doRaycast(const PxVec3& cameraPosition, const PxVec3& rayDirection, const PxReal& rayRange, int id);
};

