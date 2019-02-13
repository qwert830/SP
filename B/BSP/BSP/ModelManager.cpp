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
					
					vec2 uv = ReadUV(pMesh, iControlPointIndex, pMesh->GetTextureUVIndex(j, k));
					data.tu = uv.x;
					data.tv = uv.y;

					pOutData->push_back(data);
				}
			}

		}

	}
	return S_OK;
}