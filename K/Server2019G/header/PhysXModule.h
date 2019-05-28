#pragma once
#include "PxPhysicsAPI.h"
#include <string>
#include <vector>
using namespace physx;
using namespace std;

#define PVD_HOST "127.0.0.1"


struct ModelData
{
	float tx, ty, tz, sx, sy, sz, ry;
};

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

	void init();

	void stepPhysics(const PxReal& frame);

	//void setCapsuleController(PxCapsuleController* mCapsuleController, PxExtendedVec3 pos, float height, float radius, PxUserControllerHitReport * collisionCallback);
	PxCapsuleController* setCapsuleController(PxExtendedVec3 pos, float height, float radius, int key);

	void createBoxObj(const PxVec3& t, PxReal rotateDeg, const PxVec3& sizeofBox);

	void wlock();

	void wunlock();
	
	virtual pair<int, PxVec3> doRaycast(const PxVec3& cameraPosition, const PxVec3& rayDirection, const PxReal& rayRange, int id);
};

