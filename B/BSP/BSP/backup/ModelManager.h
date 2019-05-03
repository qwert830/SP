#pragma once
#include "d3dUtil.h"

struct ModelData 
{
	float x, y, z, w, tu, tv, nx, ny, nz;
	unsigned int index;
};

struct vec2
{
	float x, y;
};

class ModelManager
{
public:
	FbxManager * g_pFbxSdkManager = nullptr;

	ModelManager();
	~ModelManager();

	HRESULT LoadFBX(const char* filename, std::vector<ModelData>* pOutData);
	vec2 ReadUV(FbxMesh* mesh, int controlPointIndex, int vertexCounter);
	void LoadUV(FbxMesh* mesh, std::vector<ModelData>* data);
	void LoadNormal(FbxMesh* mesh, std::vector<ModelData>* data);
};

