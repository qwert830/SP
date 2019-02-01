#pragma once
#include "d3dUtil.h"

struct ModelData 
{
	float x, y, z, w, tu, tv, nx, ny, nz;
	unsigned int index;
};

class ModelManager
{
public:
	FbxManager * g_pFbxSdkManager = nullptr;

	ModelManager();
	~ModelManager();

	HRESULT LoadFBX(const char* filename, std::vector<ModelData>* pOutData);
};

