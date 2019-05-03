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
			for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex )
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