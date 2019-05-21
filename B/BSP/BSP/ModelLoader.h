#pragma once
#include "d3dUtil.h"
#include "FrameResource.h"
#include "ModelManager.h"

class ModelLoader
{
private:

public:
	ModelLoader();
	~ModelLoader();

	void LoadDataFromFile(const char* name, std::vector<ModelData>* out);

};

