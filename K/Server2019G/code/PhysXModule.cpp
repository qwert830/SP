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
	//���̰� �����Ͽ� ������ üũ�ϰ�ʹٸ� ��Ʈ���۸� �迭�� ������ ��
	PxRaycastHit buffer;
	//������ ����, �������� ����ũ��
	PxRaycastBuffer hit(&buffer, 1);
	//ù ��Ʈ�� �ٷ� �����ϴ� �ɼ��� ���Ǽ����̹Ƿ� ������ ��Ʈ��Ű�� �ʹٸ� ������ �����Ͽ� ������ �����ؾ� �� ��
	PxQueryFilterData PxQFData;
	PxQFData.flags |= PxQueryFlag::eANY_HIT;
	//����ĳ��Ʈ �Լ�
	bool status = mScene->raycast(cameraPosition, rayDirection, rayRange, hit, PxHitFlags(PxHitFlag::eDEFAULT), PxQFData);
	//���� ���ǹ��� ����ĳ��Ʈ ������ �ൿ�� �߰�, ������ ������ üũ�Ϸ��� else������ �߰�
	if (status) {
		//hit�̾ƴ϶� buffer�� �̿��� ��
		//��) buffer.actor->release();
	}
}


PxCapsuleController* PhysXModule::setCapsuleController(PxExtendedVec3 pos, float height, float radius)
{
	PxCapsuleControllerDesc capsuleDesc;
	capsuleDesc.height = height; //Height of capsule
	capsuleDesc.radius = radius; //Radius of casule
	capsuleDesc.position = pos; //Initial position of capsule
	capsuleDesc.material = mPhysics->createMaterial(1.0f, 1.0f, 1.0f); //ĸ�� ������ ����
	capsuleDesc.density = 1.0f; //ĸ�� ������ �е�?
	capsuleDesc.contactOffset = 0.001f; //����Ʈ���� �Ÿ�
	capsuleDesc.slopeLimit = 1.0f; //��縦 �ö󰡴� �̵� ����, �������� ����̵� �Ұ���
	capsuleDesc.stepOffset = 1.0f;	//��� �̵����� ����, �̺��� ���� ������Ʈ�� ������ ������ ����
	//capsuleDesc.maxJumpHeight = 2.0f; //�ִ� ���� ����
	capsuleDesc.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
	//capsuleDesc.invisibleWallHeight = 0.0f;

	//�浹 �ݹ� �Լ�
	//capsuleDesc.reportCallback = collisionCallback; //�浹�Լ��� ����� ������� �߷°����� 0����?

	capsuleDesc.reportCallback = &collisionCallback;
	//ĸ�� ��Ʈ�ѷ� ����
	return static_cast<PxCapsuleController*>(mControllerManager->createController(capsuleDesc));
	
	//ĸ�� ��Ʈ�ѷ��� key�� �ο�
	//mCapsuleController->setUserData(new int(key));
	//�ҷ��ö� ���� ���
	//reinterpret_cast<int*>(mCapsuleController->getUserData())[0];

}