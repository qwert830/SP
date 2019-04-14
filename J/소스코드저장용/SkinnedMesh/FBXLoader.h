#pragma once
#include <fbxsdk.h>
#include <vector>
#include <string>
#include <map>
#include <d3d12.h>
#include <DirectXMath.h>
#include "../../Common/DDSTextureLoader.h"

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

struct Keyframe {
	FbxLongLong mFrameNum;
	FbxAMatrix mGlobalTransform;
	Keyframe* mNext;

	Keyframe() : mNext(nullptr)
	{}
};

struct Joint {
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

class Mesh
{
public:
	Mesh(ID3D11Device *dev, std::vector<VERTEX> vertices, ID3D11ShaderResourceView *texture)
	{
		this->vertices = vertices;
		this->texture = texture;

		this->SetupMesh(dev);
	}

	void Draw(ID3D11DeviceContext *devcon)
	{
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;

		devcon->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

		if (this->texture != nullptr)
			devcon->PSSetShaderResources(0, 1, &texture);

		devcon->Draw(vertices.size(), 0);
	}
private:
	std::vector<VERTEX> vertices;
	ID3D11ShaderResourceView *texture = nullptr;
	ID3D11Buffer* vertexBuffer;

	bool SetupMesh(ID3D11Device *dev)
	{
		HRESULT hr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = sizeof(VERTEX) * vertices.size();
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &vertices[0];

		hr = dev->CreateBuffer(&vbd, &initData, &vertexBuffer);
		if (FAILED(hr))
			return false;
	}
};

class FBXLoader
{
public:
	FBXLoader();
	~FBXLoader();

	void LoadFBX(HWND hwnd, ID3D11Device *dev, ID3D11DeviceContext *devcon, const char* filename);

	void Draw(ID3D11DeviceContext *devcon);

	XMMATRIX GetAnimatedMatrix(int index);

	Skeleton skeleton;

private:
	FbxManager *fbxsdkManager = nullptr;
	FbxScene *fbxScene;
	std::map<int, int> controlpoints;
	std::vector<Mesh> meshes;
	HWND hwnd;

	void ProcessNode(ID3D11Device *dev, ID3D11DeviceContext *devcon, FbxNode *node, FbxGeometryConverter *gConverter);

	Mesh ProcessMesh(ID3D11Device* dev, ID3D11DeviceContext *devcon, FbxMesh *mesh);

	void ProcessSkeletonHeirarchy(FbxNode* rootnode);

	void ProcessSkeletonHeirarchyre(FbxNode* node, int depth, int index, int parentindex);

	unsigned int FindJointIndex(const std::string& jointname);

	ID3D11ShaderResourceView *LoadTexture(ID3D11Device *dev, ID3D11DeviceContext *devcon, const char* texturefilename);
};