#include "../header/PhysXModule.h"



PhysXModule::PhysXModule()
{
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);

#ifdef _DEBUG
	mPvd = PxCreatePvd(*mFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);
#else
	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true);
#endif

	PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	mDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	mScene = mPhysics->createScene(sceneDesc);

	mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);
	mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(PxTolerancesScale()));

	mControllerManager = PxCreateControllerManager(*mScene);
	mControllerManager->setOverlapRecoveryModule(true);

	PxRigidStatic* groundPlane = PxCreatePlane(*mPhysics, PxPlane(0, 1, 0, 0), *mMaterial);
	mScene->addActor(*groundPlane);
}

PhysXModule::~PhysXModule()
{
	mScene->release();
	mDispatcher->release();
	mPhysics->release();

#ifdef _DEBUG
	PxPvdTransport* transport = mPvd->getTransport();
	mPvd->release();
	transport->release();
#endif
	mFoundation->release();
	mMaterial->release();
	mCooking->release();
	mControllerManager->release();

}

void PhysXModule::stepPhysics(const PxReal& frame)
{
	mScene->simulate(frame);
	mScene->fetchResults(true);
}

void PhysXModule::setGravity(const PxVec3& gravityP)
{
	mScene->setGravity(gravityP);
}

void PhysXModule::doRaycast(const PxVec3& cameraPosition, const PxVec3& rayDirection, const PxReal& rayRange)
{
	//레이가 관통하여 여러번 체크하고싶다면 히트버퍼를 배열로 선언할 것
	PxRaycastHit buffer;
	//왼쪽은 버퍼, 오른쪽은 버퍼크기
	PxRaycastBuffer hit(&buffer, 1);
	//첫 히트시 바로 종료하는 옵션이 밑의설정이므로 여러번 히트시키고 싶다면 문서를 참고하여 설정을 변경해야 할 것
	PxQueryFilterData PxQFData;
	PxQFData.flags |= PxQueryFlag::eANY_HIT;
	//레이캐스트 함수
	bool status = mScene->raycast(cameraPosition, rayDirection, rayRange, hit, PxHitFlags(PxHitFlag::eDEFAULT), PxQFData);
	//밑의 조건문에 레이캐스트 성공시 행동을 추가, 실패할 경우까지 체크하려면 else문까지 추가
	if (status) {
		//hit이아니라 buffer를 이용할 것
		//예) buffer.actor->release();
	}
}


PxCapsuleController* PhysXModule::setCapsuleController(PxExtendedVec3 pos, float height, float radius)
{
	PxCapsuleControllerDesc capsuleDesc;
	capsuleDesc.height = height; //Height of capsule
	capsuleDesc.radius = radius; //Radius of casule
	capsuleDesc.position = pos; //Initial position of capsule
	capsuleDesc.material = mPhysics->createMaterial(1.0f, 1.0f, 1.0f); //캡슐 셰이프 재질
	capsuleDesc.density = 1.0f; //캡슐 셰이프 밀도?
	capsuleDesc.contactOffset = 0.001f; //콘택트판정 거리
	capsuleDesc.slopeLimit = 1.0f; //경사를 올라가는 이동 제한, 낮을수록 경사이동 불가능
	capsuleDesc.stepOffset = 1.0f;	//계단 이동가능 높이, 이보다 높은 오브젝트나 지형에 막히면 멈춤
	//capsuleDesc.maxJumpHeight = 2.0f; //최대 점프 높이
	capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
	//capsuleDesc.invisibleWallHeight = 0.0f;

	//충돌 콜백 함수
	//capsuleDesc.reportCallback = collisionCallback; //충돌함수로 지면과 닿았을때 중력가속을 0으로?

	capsuleDesc.reportCallback = &collisionCallback;
	//캡슐 컨트롤러 생성
	return static_cast<PxCapsuleController*>(mControllerManager->createController(capsuleDesc));
	
	//캡슐 컨트롤러에 key값 부여
	//mCapsuleController->setUserData(new int(key));
	//불러올때 쓰는 방법
	//reinterpret_cast<int*>(mCapsuleController->getUserData())[0];

}