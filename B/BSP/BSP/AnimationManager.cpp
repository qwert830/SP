#include "AnimationManager.h"



AnimationManager::AnimationManager()
{
}


AnimationManager::~AnimationManager()
{
}

void AnimationManager::Init()
{
	LoadDataFromFile("Resource//Walk.txt","Walk");
}

void AnimationManager::LoadDataFromFile(const char* name, const char* aniName)
{
	string arr;
	char temp[100];
	fstream f(name);
	float tx, ty, tz, tw, sx, sy, sz, sw, qx, qy, qz, qw;
	
	if (!f.is_open())
		return;
	while (true)
	{
		arr = f.get();
		if (arr == "t")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				tx = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				ty = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				tz = (float)atof(temp);
			}
			else if (arr == "w")
			{
				f.get();
				f.getline(temp, 100, ' ');
				tw = (float)atof(temp);
			}
		}
		else if (arr == "s")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				sx = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				sy = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				sz = (float)atof(temp);
			}
			else if (arr == "w")
			{
				f.get();
				f.getline(temp, 100, ' ');
				sw = (float)atof(temp);
			}
		}
		else if (arr == "q")
		{
			arr = f.get();
			if (arr == "x")
			{
				f.get();
				f.getline(temp, 100, ' ');
				qx = (float)atof(temp);
			}
			else if (arr == "y")
			{
				f.get();
				f.getline(temp, 100, ' ');
				qy = (float)atof(temp);
			}
			else if (arr == "z")
			{
				f.get();
				f.getline(temp, 100, ' ');
				qz = (float)atof(temp);
			}
			else if (arr == "w")
			{
				f.get();
				f.getline(temp, 100, ' ');
				qw = (float)atof(temp);

				auto t = XMMatrixTranslation(tx, ty, tz);
				auto s = XMMatrixScaling(sx, sy, sz);
				auto r = XMMatrixRotationQuaternion(XMLoadFloat4(&XMFLOAT4(qx, qy, qz, qw)));
				XMFLOAT4X4 result;
				XMStoreFloat4x4(&result, s*r*t);
				
				XMMATRIX temp = XMLoadFloat4x4(&result);
				XMStoreFloat4x4(&result, XMMatrixTranspose(temp));
				data[aniName].push_back(result);
			}
		}

		if (f.eof())
			return;
	}
}

int AnimationManager::GetSize(const char* aniName)
{
	return (int)data[aniName].size();
}

XMFLOAT4X4 AnimationManager::GetData(const char * aniName, int index)
{
	return data[aniName][index];
}
