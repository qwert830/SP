#include "ModelManager.h"

ModelManager::ModelManager()
{
}


ModelManager::~ModelManager()
{
}

vec2 ModelManager::ReadUV(FbxMesh* mesh, int controlPointIndex, int vertexCounter)
{
	FbxGeometryElementUV* vertexuv = mesh->GetElementUV(0);

	vec2 result;


	switch (vertexuv->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexuv->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexuv->GetDirectArray().GetAt(controlPointIndex).mData[0]);
			result.y = static_cast<float>(vertexuv->GetDirectArray().GetAt(controlPointIndex).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexuv->GetIndexArray().GetAt(controlPointIndex);
			result.x = static_cast<float>(vertexuv->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexuv->GetDirectArray().GetAt(index).mData[1]);
		}
		break;

		default:
			break;
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexuv->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			result.x = static_cast<float>(vertexuv->GetDirectArray().GetAt(vertexCounter).mData[0]);
			result.y = static_cast<float>(vertexuv->GetDirectArray().GetAt(vertexCounter).mData[1]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexuv->GetIndexArray().GetAt(vertexCounter);
			result.x = static_cast<float>(vertexuv->GetDirectArray().GetAt(index).mData[0]);
			result.y = static_cast<float>(vertexuv->GetDirectArray().GetAt(index).mData[1]);
		}
		break;
		default:
			break;
		}
		break;
	}

	return result;
}

void ModelManager::LoadUV(FbxMesh * pMesh, std::vector<ModelData>* data)
{
	FbxGeometryElementUV* lUVElement = pMesh->GetElementUV(0);
	const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
	const int lIndexCount = (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;
	const int lPolyCount = pMesh->GetPolygonCount();

	if (data->size() < lPolyCount)
		data->resize(lPolyCount);

	if (lUVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
		{
			// build the max index array that we need to pass into MakePoly
			const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
			for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
			{
				FbxVector2 lUVValue;

				//get the index of the current vertex in control points array
				int lPolyVertIndex = pMesh->GetPolygonVertex(lPolyIndex, lVertIndex);

				//the UV index depends on the reference mode
				int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

				lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);
				//User TODO:
				//Print out the value of UV(lUVValue) or log it to a file
			}
		}
	}
	else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		int lPolyIndexCounter = 0;
		for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
		{
			// build the max index array that we need to pass into MakePoly
			const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
			for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
			{
				if (lPolyIndexCounter < lIndexCount)
				{
					FbxVector2 lUVValue;
					//the UV index depends on the reference mode
					int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

					lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);
					float u = static_cast<float>(lUVValue.mData[0]);
					float v = static_cast<float>(lUVValue.mData[1]);
					data[0][lPolyIndexCounter].tu = u;
					data[0][lPolyIndexCounter].tv = v;

					//User TODO:
					//Print out the value of UV(lUVValue) or log it to a file

					lPolyIndexCounter++;
				}
			}
		}
	}

}

void ModelManager::LoadNormal(FbxMesh * mesh, std::vector<ModelData>* data)
{
	FbxGeometryElementNormal* lNormalElement = mesh->GetElementNormal();

	if (lNormalElement)
	{
		if (lNormalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{
			for (int lVertexIndex = 0; lVertexIndex < mesh->GetControlPointsCount(); lVertexIndex++)
			{
				int lNormalIndex = 0;

				if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
					lNormalIndex = lVertexIndex;

				if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
					lNormalIndex = lNormalElement->GetIndexArray().GetAt(lVertexIndex);

				FbxVector4 lNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);

			}
		}

		else if (lNormalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int lIndexByPolygonVertex = 0;
			//Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
			for (int lPolygonIndex = 0; lPolygonIndex < mesh->GetPolygonCount(); lPolygonIndex++)
			{
				//get polygon size, you know how many vertices in current polygon.
				int lPolygonSize = mesh->GetPolygonSize(lPolygonIndex);
				//retrieve each vertex of current polygon.
				for (int i = 0; i < lPolygonSize; i++)
				{
					int lNormalIndex = 0;
					if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
						lNormalIndex = lIndexByPolygonVertex;
					else if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
						lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndexByPolygonVertex);

					FbxVector4 lNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
					data[0][lIndexByPolygonVertex].nx = static_cast<float>(lNormal.mData[0]);
					data[0][lIndexByPolygonVertex].ny = static_cast<float>(lNormal.mData[1]);
					data[0][lIndexByPolygonVertex].nz = static_cast<float>(lNormal.mData[2]);


					lIndexByPolygonVertex++;
				}
			}

		}
	}
}

HRESULT ModelManager::LoadFBX(const char* filename, std::vector<ModelData>* pOutData)
{
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");

	bool bSuccess = pImporter->Initialize(filename, -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return E_FAIL;

	FbxGeometryConverter con(g_pFbxSdkManager);
	con.Triangulate(pFbxScene, true);

	pImporter->Destroy();

	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();
	
	if (pFbxRootNode)
	{
		ProcessSkeletonHierarchyRecursively(pFbxRootNode, 0, 0);
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();
			
			for (int j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				int iNumVertices = pMesh->GetPolygonSize(j);
				assert(iNumVertices == 3);

				for (int k = 0; k < iNumVertices; k++) {
					int iControlPointIndex = pMesh->GetPolygonVertex(j, k);

					ModelData data;
					data.x = (float)pVertices[iControlPointIndex].mData[0];
					data.y = (float)pVertices[iControlPointIndex].mData[1];
					data.z = (float)pVertices[iControlPointIndex].mData[2];
					data.w = (float)pVertices[iControlPointIndex].mData[3];

					mControlPoints[iControlPointIndex] = (int)pOutData->size();

					pOutData->push_back(data);
				}
			}
			FbxMesh* mesh = (FbxMesh*)pFbxChildNode->GetMesh();
			LoadUV(mesh, pOutData);
			LoadNormal(mesh, pOutData);
		}

	}
	return S_OK;
}


FbxAMatrix ModelManager::GetGeometryTransformation(FbxNode* inNode)
{
	if (!inNode)
	{
		throw std::exception("Null for mesh geometry");
	}

	const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void ModelManager::ProcessJointsAndAnim(FbxNode* inNode, FbxMesh* inMesh, FbxScene* inFbxScene, std::vector<ModelData>* pOutData)
{
	int sum = 0;
	FbxMesh* currMesh = inMesh;
	FbxScene* mFBXScene = inFbxScene;
	unsigned int numOfDeformers = currMesh->GetDeformerCount();
	FbxAMatrix geometryTransform = GetGeometryTransformation(inNode);

	FbxImporter* lImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	const bool lImportStatus = lImporter->Initialize("Resource//Walk.fbx", -1, g_pFbxSdkManager->GetIOSettings());

	for (unsigned int deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
	{
		FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(inMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!currSkin)
		{
			continue;
		}

		unsigned int numOfClusters = currSkin->GetClusterCount();
		for (unsigned int clusterIndex = 0; clusterIndex < numOfClusters; ++clusterIndex)
		{
			FbxCluster* currCluster = currSkin->GetCluster(clusterIndex);
			std::string currJointName = currCluster->GetLink()->GetName();
			unsigned int currJointIndex = FindJointIndexUsingName(currJointName);
			FbxAMatrix transformMatrix;
			FbxAMatrix transformLinkMatrix;
			FbxAMatrix globalBindposeInverseMatrix;

			currCluster->GetTransformMatrix(transformMatrix);
			currCluster->GetTransformLinkMatrix(transformLinkMatrix);
			globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			// Update the information in mSkeleton 
			mSkeleton.mJoints[currJointIndex].mGlobalBindposeInverse = globalBindposeInverseMatrix;
			mSkeleton.mJoints[currJointIndex].mNode = currCluster->GetLink();

			// Associate each joint with the control points it affects
			unsigned int numOfIndices = currCluster->GetControlPointIndicesCount();
			sum += numOfIndices;
			for (unsigned int i = 0; i < numOfIndices; ++i)
			{
				int vertexid = mControlPoints[currCluster->GetControlPointIndices()[i]];
				
				if ((*pOutData)[vertexid].boneids.x == 0) (*pOutData)[vertexid].boneids.x = (float)currJointIndex;
				if ((*pOutData)[vertexid].boneids.y == 0) (*pOutData)[vertexid].boneids.y = (float)currJointIndex;
				if ((*pOutData)[vertexid].boneids.z == 0) (*pOutData)[vertexid].boneids.z = (float)currJointIndex;
				if ((*pOutData)[vertexid].boneids.w == 0) (*pOutData)[vertexid].boneids.w = (float)currJointIndex;
				if ((*pOutData)[vertexid].weights.x == 0) (*pOutData)[vertexid].weights.x = (float)currCluster->GetControlPointWeights()[i];
				if ((*pOutData)[vertexid].weights.y == 0) (*pOutData)[vertexid].weights.y = (float)currCluster->GetControlPointWeights()[i];
				if ((*pOutData)[vertexid].weights.z == 0) (*pOutData)[vertexid].weights.z = (float)currCluster->GetControlPointWeights()[i];
				if ((*pOutData)[vertexid].weights.w == 0) (*pOutData)[vertexid].weights.w = (float)currCluster->GetControlPointWeights()[i];


				BlendingIndexWeightPair currBlendingIndexWeightPair;
				currBlendingIndexWeightPair.mBlendingIndex = currJointIndex;
				currBlendingIndexWeightPair.mBlendingWeight = currCluster->GetControlPointWeights();

//				mControlPoints[currCluster->GetControlPointIndices()[i]]->mBlendingInfo.push_back(currBlendingIndexWeightPair);
			}

			// Get animation information
			
			FbxAnimStack* currAnimStack = (FbxAnimStack*)mFBXScene->GetSrcObject(0);
			FbxString animStackName = currAnimStack->GetName();
			mAnimationName = animStackName.Buffer();

			//FbxTakeInfo* takeInfo = mFBXScene->GetTakeInfo(animStackName);
			FbxTakeInfo* takeInfo = lImporter->GetTakeInfo(0);

			FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
			FbxTime end = takeInfo->mLocalTimeSpan.GetStop();
			mAnimationLength = (int)(end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1);
			Keyframe** currAnim = &mSkeleton.mJoints[currJointIndex].mAnimation;

			std::cout << "Bone No : " << clusterIndex << " , Bone Name : " << currJointName << std::endl;
			std::cout << "Animation Length : " << end.GetFrameCount(FbxTime::eFrames24) - start.GetFrameCount(FbxTime::eFrames24) + 1 << std::endl;
			/*std::cout << "  Transform : ";
			for (int matrix = 0; matrix < 4; ++matrix)
				std::cout << mSkeleton.mJoints[currJointIndex].mGlobalBindposeInverse.GetT()[matrix] << "   ";
			std::cout << std::endl;
			std::cout << "  Scale : ";
			for (int matrix = 0; matrix < 4; ++matrix)
				std::cout << mSkeleton.mJoints[currJointIndex].mGlobalBindposeInverse.GetS()[matrix] << "  ";
			std::cout << std::endl;
			std::cout << "  Quatanion : ";
			for (int matrix = 0; matrix < 4; ++matrix)
				std::cout << mSkeleton.mJoints[currJointIndex].mGlobalBindposeInverse.GetQ()[matrix] << "  ";
			std::cout << std::endl << std::endl << std::endl;*/
				for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames24); i <= end.GetFrameCount(FbxTime::eFrames24); ++i)
				{
					FbxTime currTime;
					currTime.SetFrame(i, FbxTime::eFrames24);
					*currAnim = new Keyframe();
					(*currAnim)->mFrameNum = i;
					FbxAMatrix currentTransformOffset = inNode->EvaluateGlobalTransform(currTime) * geometryTransform;
					(*currAnim)->mGlobalTransform = currentTransformOffset.Inverse() * currCluster->GetLink()->EvaluateGlobalTransform(currTime);

					std::cout << "Current Time : " << currTime.GetMilliSeconds() << "ms" << std::endl;
					std::cout << "  Transform : ";
					for (int matrix = 0; matrix < 4; ++matrix)
						std::cout << (*currAnim)->mGlobalTransform.GetT()[matrix] << "   ";
					std::cout << "  Scale : ";
					for (int matrix = 0; matrix < 4; ++matrix)
						std::cout << (*currAnim)->mGlobalTransform.GetS()[matrix] << "  ";
					std::cout << "  Quatanion : ";
					for (int matrix = 0; matrix < 4; ++matrix)
						std::cout << (*currAnim)->mGlobalTransform.GetQ()[matrix] << "  ";
					std::cout << std::endl  ;

					currAnim = &((*currAnim)->mNext);
				}
				I_animStack = currAnimStack;
		}
	}

	// I am adding more dummy joints if there isn't enough
	/*BlendingIndexWeightPair currBlendingIndexWeightPair;
	currBlendingIndexWeightPair.mBlendingIndex = 0;
	currBlendingIndexWeightPair.mBlendingWeight = 0;*/
	//for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr)
	//{
	//	for (unsigned int i = itr->second->mBlendingInfo.size(); i <= 4; ++i)
	//	{
	//		itr->second->mBlendingInfo.push_back(currBlendingIndexWeightPair);
	//	}
	//}
}

unsigned int ModelManager::FindJointIndexUsingName(std::string inNode)
{
	for (unsigned int i = 0; i < mSkeleton.mJoints.size(); ++i)
	{
		if (mSkeleton.mJoints[i].mName == inNode)
		{
			return i;
		}
	}
	return 0;
}

void ModelManager::ProcessSkeletonHierarchyRecursively(FbxNode* inNode, int myIndex, int inParentIndex)
{
	if (inNode->GetNodeAttribute() && inNode->GetNodeAttribute()->GetAttributeType() && inNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint currJoint;
		currJoint.mParentIndex = inParentIndex;
		currJoint.mName = inNode->GetName();
		mSkeleton.mJoints.push_back(currJoint);
	}
	for (int i = 0; i < inNode->GetChildCount(); i++)
	{
		ProcessSkeletonHierarchyRecursively(inNode->GetChild(i), (int)mSkeleton.mJoints.size(), myIndex);
	}
}

void ModelManager::ProcessControlPoints(FbxNode* inNode)
{
	FbxMesh* currMesh = inNode->GetMesh();
	/*unsigned int ctrlPointCount = currMesh->GetControlPointsCount();
	for (unsigned int i = 0; i < ctrlPointCount; ++i)
	{
		CtrlPoint* currCtrlPoint = new CtrlPoint();
		XMFLOAT3 currPosition;
		currPosition.x = static_cast(currMesh->GetControlPointAt(i).mData[0]);
		currPosition.y = static_cast(currMesh->GetControlPointAt(i).mData[1]);
		currPosition.z = static_cast(currMesh->GetControlPointAt(i).mData[2]);
		currCtrlPoint->mPosition = currPosition;
		mControlPoints = currCtrlPoint;
	}*/
}

HRESULT ModelManager::LoadAnim(const char* filename, std::vector<ModelData>* pOutData)
{
	if (g_pFbxSdkManager == nullptr)
	{
		g_pFbxSdkManager = FbxManager::Create();

		FbxIOSettings* pIOsettings = FbxIOSettings::Create(g_pFbxSdkManager, IOSROOT);
		g_pFbxSdkManager->SetIOSettings(pIOsettings);
	}

	FbxImporter* pImporter = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene = FbxScene::Create(g_pFbxSdkManager, "");
	FbxImporter* pImporter2 = FbxImporter::Create(g_pFbxSdkManager, "");
	FbxScene* pFbxScene2 = FbxScene::Create(g_pFbxSdkManager, "");

	bool bSuccess = pImporter->Initialize(filename, -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter->Import(pFbxScene);
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter2->Initialize("Resource//Walk.FBX", -1, g_pFbxSdkManager->GetIOSettings());
	if (!bSuccess) return E_FAIL;

	bSuccess = pImporter2->Import(pFbxScene2);
	if (!bSuccess) return E_FAIL;

	FbxGeometryConverter con(g_pFbxSdkManager);
	con.Triangulate(pFbxScene, true);

	pImporter->Destroy();

	FbxNode* pFbxRootNode = pFbxScene->GetRootNode();

	if (pFbxRootNode)
	{
		for (int i = 0; i < pFbxRootNode->GetChildCount(); i++)
		{
			FbxNode* pFbxChildNode = pFbxRootNode->GetChild(i);

			if (pFbxChildNode->GetNodeAttribute() == NULL)
				continue;

			FbxNodeAttribute::EType AttributeType = pFbxChildNode->GetNodeAttribute()->GetAttributeType();

			if (AttributeType != FbxNodeAttribute::eMesh)
				continue;

			FbxMesh* pMesh = (FbxMesh*)pFbxChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();

			for (int j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				int iNumVertices = pMesh->GetPolygonSize(j);
				assert(iNumVertices == 3);

				for (int k = 0; k < iNumVertices; k++) {
				}
			}
			ProcessJointsAndAnim(pFbxRootNode, pMesh, pFbxScene, pOutData);
		}

	}
	return S_OK;
}