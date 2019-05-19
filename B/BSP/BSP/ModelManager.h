#pragma once
#include "d3dUtil.h"
#include <fbxsdk.h>
#include <iostream>
#include <map>
#include <set>
using namespace DirectX;

struct VERTEX
{
	XMFLOAT3 pos;
	XMFLOAT2 tex;
	XMFLOAT4 boneids;
	XMFLOAT4 weights;

	VERTEX()
	{
		boneids = { 0, 0, 0, 0 };
		weights = { 0, 0, 0, 0 };
	}
};

struct ModelData
{
	float x, y, z, w, tu, tv, nx, ny, nz;
	int state, frameTime;
	unsigned int index;
	XMFLOAT4 boneids;
	XMFLOAT4 weights;

	ModelData()
	{
		boneids = { 0, 0, 0, 0 };
		weights = { 0, 0, 0, 0 };
	}
};

struct BlendingIndexWeightPair
{
	unsigned int mBlendingIndex;
	double * mBlendingWeight;
};

struct Keyframe
{
	FbxLongLong mFrameNum;
	FbxAMatrix mGlobalTransform;
	Keyframe* mNext;

	Keyframe() : mNext(nullptr)
	{}
};

struct Joint
{
	int mParentIndex;
	const char* mName;
	FbxAMatrix mGlobalBindposeInverse;
	Keyframe* mAnimation;
	FbxNode *mNode;

	Joint() :
		mNode(nullptr),
		mAnimation(nullptr)
	{
		mGlobalBindposeInverse.SetIdentity();
		mParentIndex = -1;
	}

	~Joint()
	{
		while (mAnimation)
		{
			Keyframe* temp = mAnimation->mNext;
			delete mAnimation;
			mAnimation = temp;
		}
	}
};

struct Skeleton {
	std::vector<Joint> mJoints;
};

struct vec2
{
	float x, y;
};

class ModelManager
{
public:
	FbxManager * g_pFbxSdkManager = nullptr;
	FbxAnimStack * I_animStack = nullptr;
	FbxString mAnimationName;
	int mAnimationLength;
	std::map<int, int> mControlPoints;
	Skeleton mSkeleton;

	ModelManager();
	~ModelManager();

	HRESULT LoadFBX(const char* filename, std::vector<ModelData>* pOutData);
	HRESULT LoadAnim(const char* filename, std::vector<ModelData>* pOutData);

	void ProcessJointsAndAnim(FbxNode* inNode, FbxMesh* inMesh, FbxScene* inFbxScene, std::vector<ModelData>* pOutData);
	FbxAMatrix GetGeometryTransformation(FbxNode* inNode);
	void ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int myIndex, int inParentIndex);
	unsigned int FindJointIndexUsingName(std::string inNode);
	void ProcessControlPoints(FbxNode* inNode);

	vec2 ReadUV(FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	void LoadUV(FbxMesh* mesh, std::vector<ModelData>* data);
	void LoadNormal(FbxMesh* mesh, std::vector<ModelData>* data);
};