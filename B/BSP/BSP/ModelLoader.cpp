#include "ModelLoader.h"



ModelLoader::ModelLoader()
{
}


ModelLoader::~ModelLoader()
{
}

void ModelLoader::LoadDataFromFile(const char * name, std::vector<ModelData>* out)
{
	float x, y, z, u, v, nx, ny, nz, b1, b2, b3, b4, w1, w2, w3, w4;
	char temp[100];
	std::string arr;
	std::fstream f(name);
	if (!f.is_open())
		return;

	int i = 0;
	ModelData d;
	while (true)
	{
		arr = f.get();
		if (arr == "x")
		{
			arr = f.get();
			f.getline(temp, 100, ' ');
			d.x = (float)atof(temp);
		}
		else if (arr == "y")
		{
			arr = f.get();
			f.getline(temp, 100, ' ');
			d.y = (float)atof(temp);
		}
		else if (arr == "z")
		{
			arr = f.get();
			f.getline(temp, 100, ' ');
			d.z = (float)atof(temp);
		}
		else if (arr == "t")
		{
			arr = f.get();
			if (arr == "u")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.tu = (float)atof(temp);
			}
			else if (arr == "v")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.tv = (float)atof(temp);
			}
		}
		else if (arr == "n")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.nx = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.ny = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.nz = (float)atof(temp);
			}
		}
		else if (arr == "b")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.boneids.x = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.boneids.y = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.boneids.z = (float)atof(temp);
			}
			else if (arr == "a")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.boneids.w = (float)atof(temp);
			}
		}
		else if (arr == "w")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.weights.x = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.weights.y = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.weights.z = (float)atof(temp);
			}
			else if (arr == "a")
			{
				f.get();
				f.getline(temp, 100, ' ');
				d.weights.w = (float)atof(temp);
				out->push_back(d);
			}
		}

		if (f.eof())
			return;

	}
}
