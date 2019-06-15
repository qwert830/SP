#pragma once

#ifdef max

#undef max
#undef min
#endif

#include "assimp\Importer.hpp"
#include "assimp\cimport.h"
#include "assimp\postprocess.h"
#include "assimp\scene.h"

#include "FrameResource.h"
#include "d3dUtil.h"

#pragma comment(lib, "assimp-vc140-mt.lib")

enum AnimationName
{
	IDLE = 0, FIRE, RUN, DEATH
};



using namespace std;

/*
	aiScene = Mesh, Rootnode, Animation
		Mesh->aiMesh = Name, [normal], face, [bone], [vertice]
			[[bone->aiBone = Offsetmatrix, weight, Name]]
		Rootnode->aiNode = Child(Child->aiNode), [[Meshindex(->aiMesh)]], Name, Matrix
		
		Animation->aiAnimation = Name, channel->aiNodeAnim, Duration, Ticks per second
			[[aiNodeAnim = Name, RotKey(회전), ScaleKey, PosKey]] / 전부 Time, value값을 가짐
*/

struct mesh
{
	vector<Vertex> m_vertices;
	vector<int>	m_indices;
};

struct Bone
{
	XMMATRIX BoneOffset = XMMatrixIdentity();
	XMMATRIX TransFormation = XMMatrixIdentity();
};

class ModelLoader
{
private:
	const aiScene*				m_pScene;
	const aiScene*				m_pAnimScene[10];
	
	unsigned int				m_AnimationType[10] = { IDLE, };
	float						m_CurrentAnimationTime[10] = { 0, };
	
	vector<mesh>				m_Meshes;
	vector<pair<string, Bone>>	m_Bones;

	unsigned int m_NumVertices = -1;
	unsigned int m_NumBones = -1;

	XMMATRIX m_GlobalInverseTransform;

public:
	//매쉬정보, 본 
	ModelLoader();
	~ModelLoader();
	void InitScene();
	void InitMesh(unsigned int index, const aiMesh* pMesh);
	void InitBone(unsigned int index, const aiMesh* pMesh);
	void ModelLoad(const string& file, bool isStatic);

	vector<mesh> GetMesh();

	//애니메이션
	void InitAnimation();
	void GetAnimation();

	void AnimationLoad(const string& file, unsigned index);
	void BoneTransform(XMFLOAT4X4* Transforms, int index);

	void ReadNodeHeirarchy(float AnimationTime, const aiNode* pNose, const XMMATRIX& ParentTransform, int AnimationType);
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimaition, const string& NodeName);

	void CalcInterpolatedScaling(aiVector3D& Scaling, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedRotation(aiQuaternion& RotationQ, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void CalcInterpolatedPosition(aiVector3D& Translation, float AnimationTime, const aiNodeAnim* pNodeAnim);

	unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);

	//애니메이션 설정 및 업데이트

	void ChangeAnimation(int index, int AnimationType);
	unsigned int GetAnimationType(int index);
	void UpdateTime(float dt);

};


