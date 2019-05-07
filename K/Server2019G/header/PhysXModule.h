#pragma once
#include "PxPhysicsAPI.h"
#include <string>
using namespace physx;
using namespace std;

#define PVD_HOST "127.0.0.1"

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
	
#ifdef _DEBUG
	PxPvd*                  mPvd = NULL;
#endif
public:
	PhysXModule();
	~PhysXModule();

	void stepPhysics(const PxReal& frame);
	void setGravity(const PxVec3& gravityP);

	//void setCapsuleController(PxCapsuleController* mCapsuleController, PxExtendedVec3 pos, float height, float radius, PxUserControllerHitReport * collisionCallback);
	void setCapsuleController(PxCapsuleController * mCapsuleController, PxExtendedVec3 pos, float height, float radius);
	
	virtual void doRaycast(const PxVec3& cameraPosition, const PxVec3& rayDirection, const PxReal& rayRange);
};

