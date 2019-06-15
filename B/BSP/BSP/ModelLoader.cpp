#include "ModelLoader.h"



ModelLoader::ModelLoader()
{
}

ModelLoader::~ModelLoader()
{
}

void ModelLoader::InitScene()
{
	for (unsigned int i = 0; i < m_Meshes.size(); ++i)
	{
		const aiMesh* pMesh = m_pScene->mMeshes[i];
		InitMesh(i, pMesh);
		m_NumVertices += (unsigned int)m_Meshes[i].m_vertices.size();
	}

}

void ModelLoader::InitMesh(unsigned int index, const aiMesh * pMesh)
{
	m_Meshes[index].m_vertices.reserve(pMesh->mNumVertices);
	m_Meshes[index].m_indices.reserve(pMesh->mNumFaces*3);

	for (UINT i = 0; i < pMesh->mNumVertices; ++i) {

		XMFLOAT3 pos(&pMesh->mVertices[i].x);

		XMFLOAT3 normal(&pMesh->mNormals[i].x);

		XMFLOAT2 tex;

		if (pMesh->HasTextureCoords(0))
			tex = XMFLOAT2(&pMesh->mTextureCoords[0][i].x);
		else
			tex = XMFLOAT2(0.0f, 0.0f);

		const Vertex data(XMFLOAT4(pos.x, pos.y, pos.z, 1.0f), normal, tex);

		m_Meshes[index].m_vertices.push_back(data);
	}
	for (UINT i = 0; i < pMesh->mNumFaces; ++i) {

		const aiFace& face = pMesh->mFaces[i];

		m_Meshes[index].m_indices.push_back(face.mIndices[0]);
		m_Meshes[index].m_indices.push_back(face.mIndices[1]);
		m_Meshes[index].m_indices.push_back(face.mIndices[2]);
	}
}

void ModelLoader::InitBone(unsigned int index, const aiMesh* pMesh)
{
	for (unsigned int i = 0; i < pMesh->mNumBones; ++i)
	{
		aiBone* bone = pMesh->mBones[i];
		auto numOfVertex = bone->mNumWeights;
		for (unsigned int b = 0; b < numOfVertex; ++b)
		{
			unsigned int id = bone->mWeights[b].mVertexId;
			if (m_Meshes[index].m_vertices[id].BoneWeights.x == 0.0f)
			{
				m_Meshes[index].m_vertices[id].BoneWeights.x = bone->mWeights[b].mWeight;
				m_Meshes[index].m_vertices[id].BoneIndices[0] = i;
			}
			else if (m_Meshes[index].m_vertices[id].BoneWeights.y == 0.0f)
			{
				m_Meshes[index].m_vertices[id].BoneWeights.y = bone->mWeights[b].mWeight;
				m_Meshes[index].m_vertices[id].BoneIndices[1] = i;
			}
			else if (m_Meshes[index].m_vertices[id].BoneWeights.z == 0.0f)
			{
				m_Meshes[index].m_vertices[id].BoneWeights.z = bone->mWeights[b].mWeight;
				m_Meshes[index].m_vertices[id].BoneIndices[2] = i;
			}
			else
			{
				m_Meshes[index].m_vertices[id].BoneIndices[3] = i;
			}
		}
		m_NumBones++;
		Bone bData;
		bData.BoneOffset = XMMATRIX(&bone->mOffsetMatrix.a1);
		m_Bones.push_back(make_pair(bone->mName.data, bData));
	}
	int k = 0;
}

void ModelLoader::ModelLoad(const std::string & file, bool isStatic)
{
	UINT flag = 
		aiProcess_JoinIdenticalVertices |			// join identical vertices/ optimize indexing
		aiProcess_ValidateDataStructure |			// perform a full validation of the loader's output
		aiProcess_ImproveCacheLocality |			// improve the cache locality of the output vertices
		aiProcess_RemoveRedundantMaterials |		// remove redundant materials
		aiProcess_GenUVCoords |						// convert spherical, cylindrical, box and planar mapping to proper UVs
		aiProcess_TransformUVCoords |				// pre-process UV transformations (scaling, translation ...)
		aiProcess_FindInstances |					// search for instanced meshes and remove them by references to one master
		aiProcess_LimitBoneWeights |				// limit bone weights to 4 per vertex
		aiProcess_OptimizeMeshes |					// join small meshes, if possible;
		aiProcess_GenSmoothNormals |				// generate smooth normal vectors if not existing
		aiProcess_SplitLargeMeshes |				// split large, unrenderable meshes into sub-meshes
		aiProcess_Triangulate |						// triangulate polygons with more than 3 edges
		aiProcess_ConvertToLeftHanded |				// convert everything to D3D left handed space
		aiProcess_SortByPType;						// make 'clean' meshes which consist of a single type of primitives

	if (isStatic)
		flag |= aiProcess_PreTransformVertices;			// preTransform Vertices (no bone & animation flag)

	m_pScene = aiImportFile(file.c_str(), flag);
	auto globalInverseTransform = m_pScene->mRootNode->mTransformation;
	globalInverseTransform.Inverse();

	m_GlobalInverseTransform = XMMATRIX(&globalInverseTransform.a1);

	if (m_pScene) {
		m_Meshes.resize(m_pScene->mNumMeshes);
		m_NumBones = 0;
		InitScene();

		for (int i = 0; i < m_Meshes.size(); ++i)
		{
			const aiMesh* pMesh = m_pScene->mMeshes[i];
			if (pMesh->HasBones())
			{
				InitBone(i, pMesh);
			}
		}
	}
}

vector<mesh> ModelLoader::GetMesh()
{
	return m_Meshes;
}

void ModelLoader::InitAnimation()
{
	AnimationLoad("Resource//Idle_Rifle.FBX", IDLE);
	AnimationLoad("Resource//Fire_1Pistol.FBX", FIRE);
	AnimationLoad("Resource//Run_Rifle.FBX", RUN);
	AnimationLoad("Resource//Death_Rifle.FBX", DEAD);
}

void ModelLoader::GetAnimation()
{
}

void ModelLoader::AnimationLoad(const string & file, unsigned index)
{
	UINT flag = (aiProcessPreset_TargetRealtime_Quality | aiProcess_ConvertToLeftHanded) & ~aiProcess_FindInvalidData;
	m_pAnimScene[index] = aiImportFile(file.c_str(), flag);
}

void ModelLoader::BoneTransform(XMFLOAT4X4* Transforms, int index)
{
	XMMATRIX Identity;
	Identity = XMMatrixIdentity();

	float TicksPerSecond = (float)(m_pAnimScene[m_AnimationType[index]]->mAnimations[0]->mTicksPerSecond != 0 ?
		m_pAnimScene[m_AnimationType[index]]->mAnimations[0]->mTicksPerSecond : 25.0f);
	float TimeInTicks = m_CurrentAnimationTime[index] * TicksPerSecond;
	float AnimationTime = (float)fmod(TimeInTicks, m_pAnimScene[m_AnimationType[index]]->mAnimations[0]->mDuration);

	ReadNodeHeirarchy(AnimationTime, m_pAnimScene[m_AnimationType[index]]->mRootNode, Identity, index);

	for (unsigned int i = 0; i < m_NumBones; i++)
	{
		XMStoreFloat4x4(&Transforms[i], m_Bones[i].second.TransFormation);
	}
}

void ModelLoader::ReadNodeHeirarchy(float AnimationTime, const aiNode * pNode, const XMMATRIX & ParentTransform, int index)
{
	string NodeName(pNode->mName.data);

	const aiAnimation* pAnimation = m_pAnimScene[m_AnimationType[index]]->mAnimations[0];
	
	XMMATRIX NodeTransformation = XMMATRIX(&pNode->mTransformation.a1);

	const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

	if (pNodeAnim)
	{
		aiVector3D Scaling;
		CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
		XMMATRIX ScalingM;
		ScalingM = XMMatrixScaling(Scaling.x, Scaling.y, Scaling.z);

		aiQuaternion RotationQ;
		CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
		XMMATRIX RotationM;
		RotationM = XMMatrixRotationQuaternion(XMVectorSet(RotationQ.x, RotationQ.y, RotationQ.z, RotationQ.w));

		aiVector3D Translation;
		CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
		XMMATRIX TranslationM;
		TranslationM = XMMatrixTranslation(Translation.x, Translation.y, Translation.z);

		NodeTransformation = ScalingM * RotationM * TranslationM;
		NodeTransformation = XMMatrixTranspose(NodeTransformation);
	}

	XMMATRIX GlobalTransformation = ParentTransform * NodeTransformation;

	for (auto& p : m_Bones)
	{
		if (p.first == pNode->mName.data)
		{
			p.second.TransFormation = m_GlobalInverseTransform * GlobalTransformation * p.second.BoneOffset;
			break;
		}
	}

	for (unsigned int i = 0; i < pNode->mNumChildren; ++i) {
		//계층구조를 이룸. 자식노드 탐색 및 변환
		ReadNodeHeirarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation, index);
	}

}

const aiNodeAnim * ModelLoader::FindNodeAnim(const aiAnimation * pAnimaition, const string & NodeName)
{
	for (unsigned int i = 0; i < pAnimaition->mNumChannels; ++i)
	{
		const aiNodeAnim* pNodeAnim = pAnimaition->mChannels[i];

		if (pNodeAnim->mNodeName.data == NodeName)
			return pNodeAnim;
	}

	return nullptr;
}

void ModelLoader::CalcInterpolatedScaling(aiVector3D & Scaling, float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	if (pNodeAnim->mNumScalingKeys == 1)
	{
		Scaling = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}
	unsigned int ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
	unsigned int NextScalingIndex = ScalingIndex + 1;
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);

	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	//Factor은 0~1 사이값 동일한 애니메이션 시간으로 0이거나 DeltaTime과 동일한 시간이되어 1의 최고값을 가짐 
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime; 
	const aiVector3D& StartScaling = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& EndScaling = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D s = EndScaling - StartScaling;

	Scaling = StartScaling + s * Factor;
}

void ModelLoader::CalcInterpolatedRotation(aiQuaternion & RotationQ, float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	if (pNodeAnim->mNumRotationKeys == 1)
	{
		RotationQ = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}
	unsigned int RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	unsigned int NextRotationIndex = RotationIndex + 1;
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);

	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	//Factor은 0~1 사이값 동일한 애니메이션 시간으로 0이거나 DeltaTime과 동일한 시간이되어 1의 최고값을 가짐 
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	aiQuaternion::Interpolate(RotationQ, StartRotationQ, EndRotationQ, Factor);
	RotationQ = RotationQ.Normalize();
}

void ModelLoader::CalcInterpolatedPosition(aiVector3D & Translation, float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	if (pNodeAnim->mNumPositionKeys == 1)
	{
		Translation = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}
	unsigned int PositionIndex = FindPosition(AnimationTime, pNodeAnim);
	unsigned int NextPositionIndex = PositionIndex + 1;
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);

	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	//Factor은 0~1 사이값 동일한 애니메이션 시간으로 0이거나 DeltaTime과 동일한 시간이되어 1의 최고값을 가짐 
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	const aiVector3D& StartPosition = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& EndPosition = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	aiVector3D s = EndPosition - StartPosition;

	Translation = StartPosition + s * Factor;
}

unsigned int ModelLoader::FindScaling(float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
	{
		// 애니메이션 시간보다 스케일링 키에 시간이 커지는 경우는 다음애니메이션으로 넘어가는 경우
		// 따라서 i번째가 현재 진행중인 애니메이션
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
			return i;
	}

	return 0;
}

unsigned int ModelLoader::FindRotation(float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
			return i;
	}

	return 0;
}

unsigned int ModelLoader::FindPosition(float AnimationTime, const aiNodeAnim * pNodeAnim)
{
	for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
	{
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
			return i;
	}

	return 0;
}

void ModelLoader::ChangeAnimation(int index, int AnimationType)
{
	if (m_AnimationType[index] == AnimationType)
		return;
	m_AnimationType[index] = AnimationType;
	m_CurrentAnimationTime[index] = 0;
}

unsigned int ModelLoader::GetAnimationType(int index)
{
	return m_AnimationType[index];
}

void ModelLoader::UpdateTime(float dt)
{
	for (int i = 0; i < 10; ++i)
	{
		if (m_AnimationType[i] == DEAD && m_CurrentAnimationTime[i] >= 3.3f)
			continue;
		m_CurrentAnimationTime[i] += dt;
	}
}
