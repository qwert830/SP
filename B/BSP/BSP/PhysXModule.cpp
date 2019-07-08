#include "PhysXModule.h"


PhysXModule::PhysXModule()
{
	init();
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

void PhysXModule::init()
{
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);

#ifdef _DEBUG
	mPvd = PxCreatePvd(*mFoundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
	mPvd->connect(*transport, PxPvdInstrumentationFlag::eDEBUG);

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

}

void PhysXModule::stepPhysics(const PxReal& frame)
{
	mScene->lockWrite();
	mScene->lockRead();
	mScene->simulate(frame);
	mScene->fetchResults(true);
	mScene->unlockRead();
	mScene->unlockWrite();

}

pair<int, PxVec3> PhysXModule::doRaycast(const PxVec3& cameraPosition, const PxVec3& rayDirection, const PxReal& rayRange, int id)
{

	//레이가 관통하여 여러번 체크하고싶다면 히트버퍼를 배열로 선언할 것
	PxRaycastHit hits[30];
	//왼쪽은 버퍼, 오른쪽은 버퍼갯수
	PxRaycastBuffer buf(hits, 30);
	//레이캐스트 함수
	mScene->lockWrite();
	bool status = mScene->raycast(cameraPosition, rayDirection, rayRange, buf, PxHitFlag::ePOSITION);
	mScene->unlockWrite();
	//밑의 조건문에 레이캐스트 성공시 행동을 추가, 실패할 경우까지 체크하려면 else문까지 추가
	if (status) {
		//buf이아니라 hits를 이용할 것
		//예) hits.actor->release();
		float compare(rayRange);
		float distance = rayRange;
		int residx = -1;
		for (int i = 0; i < buf.nbTouches; ++i) {
			if (buf.touches[i].actor->userData) {
				if (id != reinterpret_cast<int*>(buf.touches[i].actor->userData)[0]) {
					distance = sqrtf(((buf.touches[i].position.x - cameraPosition.x)*(buf.touches[i].position.x - cameraPosition.x)) +
						((buf.touches[i].position.y - cameraPosition.y)*(buf.touches[i].position.y - cameraPosition.y)) +
						((buf.touches[i].position.z - cameraPosition.z)*(buf.touches[i].position.z - cameraPosition.z)));
					if (compare > distance) {
						compare = distance;
						residx = i;
					}
				}
			}
		}
		if(residx != -1)
			return pair<int, PxVec3>(reinterpret_cast<int*>(buf.touches[residx].actor->userData)[0], buf.touches[residx].position);
	}
	return pair<int, PxVec3>(-1, PxVec3(0, 0, 0));
}


PxCapsuleController* PhysXModule::setCapsuleController(PxExtendedVec3 pos, float height, float radius, int key)
{
	PxCapsuleControllerDesc capsuleDesc;
	capsuleDesc.height = height; //Height of capsule
	capsuleDesc.radius = radius; //Radius of casule
	capsuleDesc.position = pos; //Initial position of capsule
	capsuleDesc.material = mPhysics->createMaterial(1.0f, 1.0f, 1.0f); //캡슐 셰이프 재질
	capsuleDesc.density = 1.0f; //캡슐 셰이프 밀도?
	capsuleDesc.contactOffset = 0.01f; //콘택트판정 거리
	capsuleDesc.slopeLimit = 0.1f; //경사를 올라가는 이동 제한, 낮을수록 경사이동 불가능
	capsuleDesc.stepOffset = 0.1f;	//계단 이동가능 높이, 이보다 높은 오브젝트나 지형에 막히면 멈춤
	//capsuleDesc.maxJumpHeight = 2.0f; //최대 점프 높이
	capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
	//capsuleDesc.invisibleWallHeight = 0.0f;

	//충돌 콜백 함수
	capsuleDesc.reportCallback = &collisionCallback;

	PxCapsuleController* PC = static_cast<PxCapsuleController*>(mControllerManager->createController(capsuleDesc));

	//캡슐 컨트롤러에 유저정보 부여
	PC->getActor()->userData = (new int(key));
	//불러올때 쓰는 방법
	//reinterpret_cast<저장한데이터형*>(mCapsuleController->getUserData())[0];

	
	//캡슐 컨트롤러 생성
	return PC;
	
}

void PhysXModule::createBoxObj(const PxVec3& pos, PxReal rotateDeg, const PxVec3& sizeofBox)
{
	PxShape* shape = mPhysics->createShape(PxBoxGeometry(sizeofBox.x, sizeofBox.y, sizeofBox.z), *mMaterial);

	PxMat33 rot = PxMat33(PxIdentity);
	rot[0][0] = rot[2][2] = cosf(rotateDeg * (3.141592f / 180.0f));
	rot[0][2] = -sinf(rotateDeg * (3.141592f / 180.0f));
	rot[2][0] = sinf(rotateDeg * (3.141592f / 180.0f));
	const PxQuat rotation(rot);
	PxTransform tmp(pos, rotation);
	PxRigidStatic* obj = mPhysics->createRigidStatic(tmp);
	obj->attachShape(*shape);
	obj->userData = (new int(-2));
	mScene->addActor(*obj);
	shape->release();
}