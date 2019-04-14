#include "FBXLoader.h"



FBXLoader::FBXLoader()
{
}


FBXLoader::~FBXLoader()
{
}

void FBXLoader::LoadFBX(HWND hwnd, ID3D11Device * dev, ID3D11DeviceContext *devcon, const char * filename)
{
	if (fbxsdkManager == nullptr)
	{
		fbxsdkManager = FbxManager::Create();

		FbxIOSettings* ioSettings = FbxIOSettings::Create(fbxsdkManager, IOSROOT);
		fbxsdkManager->SetIOSettings(ioSettings);
	}

	FbxImporter *importer = FbxImporter::Create(fbxsdkManager, "");
	fbxScene = FbxScene::Create(fbxsdkManager, "");

	FbxGeometryConverter gConverter(fbxsdkManager);

	bool bSuccess = importer->Initialize(filename, -1, fbxsdkManager->GetIOSettings());

	bSuccess = importer->Import(fbxScene);

	importer->Destroy();

	FbxNode *fbxRootNode = fbxScene->GetRootNode();

	ProcessSkeletonHeirarchy(fbxRootNode);

	this->hwnd = hwnd;

	ProcessNode(dev, devcon, fbxRootNode, &gConverter);
}

void FBXLoader::Draw(ID3D11DeviceContext * devcon)
{
	for (int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Draw(devcon);
	}
}

XMMATRIX FBXLoader::GetAnimatedMatrix(int index)
{
	XMMATRIX bonematxm;
	FbxAMatrix bonemat = skeleton.mJoints[index].mGlobalBindposeInverse; //* skeleton.mJoints[0].mAnimation->mGlobalTransform;

	bonematxm = XMMatrixTranslation(bonemat.GetT().mData[0], bonemat.GetT().mData[1], bonemat.GetT().mData[2]);
	bonematxm *= XMMatrixRotationX(bonemat.GetR().mData[0]);
	bonematxm *= XMMatrixRotationY(bonemat.GetR().mData[1]);
	bonematxm *= XMMatrixRotationZ(bonemat.GetR().mData[2]);

	return bonematxm;
}

void FBXLoader::ProcessNode(ID3D11Device * dev, ID3D11DeviceContext *devcon, FbxNode * node, FbxGeometryConverter * gConverter)
{
	if (node)
	{
		if (node->GetNodeAttribute() != nullptr)
		{
			FbxNodeAttribute::EType AttributeType = node->GetNodeAttribute()->GetAttributeType();

			if (AttributeType == FbxNodeAttribute::eMesh)
			{
				FbxMesh *mesh;

				mesh = (FbxMesh*)gConverter->Triangulate(node->GetNodeAttribute(), true);

				meshes.push_back(ProcessMesh(dev, devcon, mesh));
			}
		}

		for (int i = 0; i < node->GetChildCount(); i++)
		{
			ProcessNode(dev, devcon, node->GetChild(i), gConverter);
		}
	}
}

Mesh FBXLoader::ProcessMesh(ID3D11Device * dev, ID3D11DeviceContext *devcon, FbxMesh * mesh)
{
	std::vector<VERTEX> meshvertices;
	ID3D11ShaderResourceView *meshtexture = nullptr;

	FbxVector4 *vertices = mesh->GetControlPoints();

	for (int j = 0; j < mesh->GetPolygonCount(); j++)
	{
		int numVertices = mesh->GetPolygonSize(j);

		FbxLayerElementArrayTemplate<FbxVector2> *uvVertices = 0;
		mesh->GetTextureUV(&uvVertices, FbxLayerElement::eTextureDiffuse);

		for (int k = 0; k < numVertices; k++)
		{
			int controlPointIndex = mesh->GetPolygonVertex(j, k);

			VERTEX vertex;

			vertex.pos.x = (float)vertices[controlPointIndex].mData[0];
			vertex.pos.y = (float)vertices[controlPointIndex].mData[1];
			vertex.pos.z = (float)vertices[controlPointIndex].mData[2];

			vertex.tex.x = (float)uvVertices->GetAt(mesh->GetTextureUVIndex(j, k)).mData[0];
			vertex.tex.y = 1.0f - (float)uvVertices->GetAt(mesh->GetTextureUVIndex(j, k)).mData[1];

			controlpoints[controlPointIndex] = meshvertices.size();

			meshvertices.push_back(vertex);
		}
	}

	int materialcount = mesh->GetNode()->GetSrcObjectCount<FbxSurfaceMaterial>();

	for (int i = 0; i < materialcount; i++)
	{
		FbxSurfaceMaterial *material = (FbxSurfaceMaterial*)mesh->GetNode()->GetSrcObject<FbxSurfaceMaterial>(i);

		if (material)
		{
			FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

			const FbxTexture* texture = FbxCast<FbxTexture>(prop.GetSrcObject<FbxTexture>(0));
			const FbxFileTexture* filetexture = FbxCast<FbxFileTexture>(texture);

			ID3D11ShaderResourceView *meshctexture = LoadTexture(dev, devcon, filetexture->GetFileName());

			meshtexture = meshctexture;
		}
	}

	const FbxVector4 lT = mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot);

	FbxAMatrix geometryTransform = FbxAMatrix(lT, lR, lS);

	for (unsigned int deformerIndex = 0; deformerIndex < mesh->GetDeformerCount(); ++deformerIndex)
	{
		FbxSkin* skin = reinterpret_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
		if (!skin)
			continue;

		for (unsigned int clusterIndex = 0; clusterIndex < skin->GetClusterCount(); ++clusterIndex)
		{
			FbxCluster* cluster = skin->GetCluster(clusterIndex);
			std::string jointname = cluster->GetLink()->GetName();
			unsigned int jointIndex = FindJointIndex(jointname);
			FbxAMatrix transformMatrix;
			FbxAMatrix transformLinkMatrix;
			FbxAMatrix globalBindposeInverseMatrix;

			cluster->GetTransformMatrix(transformMatrix);
			cluster->GetTransformLinkMatrix(transformLinkMatrix);
			globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix * geometryTransform;

			skeleton.mJoints[jointIndex].mGlobalBindposeInverse = globalBindposeInverseMatrix;
			skeleton.mJoints[jointIndex].mNode = cluster->GetLink();

			for (unsigned int i = 0; i < cluster->GetControlPointIndicesCount(); ++i)
			{
				int vertexid = controlpoints[cluster->GetControlPointIndices()[i]];

				if (meshvertices[vertexid].boneids.x == 0) meshvertices[vertexid].boneids.x = jointIndex;
				if (meshvertices[vertexid].boneids.y == 0) meshvertices[vertexid].boneids.y = jointIndex;
				if (meshvertices[vertexid].boneids.z == 0) meshvertices[vertexid].boneids.z = jointIndex;
				if (meshvertices[vertexid].boneids.w == 0) meshvertices[vertexid].boneids.w = jointIndex;
				if (meshvertices[vertexid].weights.x == 0) meshvertices[vertexid].weights.x = cluster->GetControlPointWeights()[i];
				if (meshvertices[vertexid].weights.y == 0) meshvertices[vertexid].weights.y = cluster->GetControlPointWeights()[i];
				if (meshvertices[vertexid].weights.z == 0) meshvertices[vertexid].weights.z = cluster->GetControlPointWeights()[i];
				if (meshvertices[vertexid].weights.w == 0) meshvertices[vertexid].weights.w = cluster->GetControlPointWeights()[i];
			}

			FbxAnimStack* animstack = fbxScene->GetSrcObject<FbxAnimStack>(0);
			FbxString animstackname = animstack->GetName();
			FbxTakeInfo* takeinfo = fbxScene->GetTakeInfo(animstackname);
			FbxTime start = takeinfo->mLocalTimeSpan.GetStart();
			FbxTime end = takeinfo->mLocalTimeSpan.GetStop();
			FbxLongLong animationlength = end.GetFrameCount(FbxTime::eFrames30) - start.GetFrameCount(FbxTime::eFrames30) + 1;
			Keyframe** anim = &skeleton.mJoints[jointIndex].mAnimation;

			for (FbxLongLong i = start.GetFrameCount(FbxTime::eFrames30); i <= end.GetFrameCount(FbxTime::eFrames30); ++i)
			{
				FbxTime time;
				time.SetFrame(i, FbxTime::eFrames30);
				*anim = new Keyframe();
				(*anim)->mFrameNum = i;
				FbxAMatrix transformoffset = mesh->GetNode()->EvaluateGlobalTransform(1.0f) * geometryTransform;
				(*anim)->mGlobalTransform = transformoffset.Inverse() * cluster->GetLink()->EvaluateGlobalTransform(time);
				anim = &((*anim)->mNext);
			}
		}
	}

	return Mesh(dev, meshvertices, meshtexture);
}

void FBXLoader::ProcessSkeletonHeirarchy(FbxNode * rootnode)
{
	for (int childindex = 0; childindex < rootnode->GetChildCount(); ++childindex)
	{
		FbxNode *node = rootnode->GetChild(childindex);
		ProcessSkeletonHeirarchyre(node, 0, 0, -1);
	}
}

void FBXLoader::ProcessSkeletonHeirarchyre(FbxNode * node, int depth, int index, int parentindex)
{
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		Joint joint;
		joint.mParentIndex = parentindex;
		joint.mName = node->GetName();
		skeleton.mJoints.push_back(joint);
	}
	for (int i = 0; i < node->GetChildCount(); i++)
	{
		ProcessSkeletonHeirarchyre(node->GetChild(i), depth + 1, skeleton.mJoints.size(), index);
	}
}

unsigned int FBXLoader::FindJointIndex(const std::string & jointname)
{
	for (unsigned int i = 0; i < skeleton.mJoints.size(); ++i)
	{
		if (skeleton.mJoints[i].mName == jointname)
		{
			return i;
		}
	}
}

ID3D11ShaderResourceView * FBXLoader::LoadTexture(ID3D11Device * dev, ID3D11DeviceContext * devcon, const char * texturefilename)
{
	HRESULT hr;

	ID3D11ShaderResourceView *texture;

	std::string filenamestr(texturefilename);
	std::string sl = "/";
	size_t start_pos = filenamestr.find(sl);
	filenamestr.replace(start_pos, sl.length(), "\\");
	std::wstring filename = std::wstring(filenamestr.begin(), filenamestr.end());

	hr = CreateDDSTextureFromFile(dev, devcon, filename.c_str(), nullptr, &texture);
	if (FAILED(hr))
		return nullptr;

	return texture;
}