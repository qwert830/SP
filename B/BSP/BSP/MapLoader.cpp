#include "MapLoader.h"

MapLoader::MapLoader()
{
}


MapLoader::~MapLoader()
{
}

void MapLoader::LoadData()
{
	LoadMapData();
	LoadPlayerData();
}

void MapLoader::LoadMapData()
{
	std::string arr;
	char temp[100];
	float offsetX[100];
	float offsetY[100];
	float offsetZ[100];
	float scalingX[100];
	float scalingY[100];
	float scalingZ[100];
	float rotationY[100];

	std::fstream f;
	f.open("Resource/Map.txt");
	if (!f.is_open())
		return;

	int i = 0;
	while (1)
	{
		arr = f.get();
		if (arr == "t")
		{
			arr = f.get();
			if (arr == "x")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				offsetX[i] = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				offsetY[i] = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				offsetZ[i] = (float)atof(temp);
			}
		}
		else if (arr == "s")
		{
			arr = f.get();
			if (arr == "x")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				scalingX[i] = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				scalingY[i] = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				scalingZ[i] = (float)atof(temp);
			}
		}
		else if (arr == "r")
		{
			arr = f.get();
			arr = f.get();
			f.getline(temp, 100, ' ');
			rotationY[i] = (float)atof(temp);
			mMapInfo.push_back(MapData(offsetX[i], offsetY[i], offsetZ[i], scalingX[i], scalingY[i], scalingZ[i], rotationY[i]));
			i++;
		}
		
		if (f.eof())
			return;
	}

}

void MapLoader::LoadPlayerData()
{
	std::string arr;
	char temp[100];
	float tX[100];
	float tY[100];
	float tZ[100];

	std::fstream f;
	f.open("Resource/PlayerMap.txt");
	if (!f.is_open())
		return;

	int i = 0;
	while (1)
	{
		arr = f.get();
		if (arr == "t")
		{
			arr = f.get();
			if (arr == "x")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				tX[i] = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				tY[i] = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				tZ[i] = (float)atof(temp);
				mPlayerInfo.push_back(PlayerData(tX[i], tY[i], tZ[i]));
				i++;
			}
		}
		if (f.eof())
			return;
	}
}

int MapLoader::GetSizeofMapData()
{
	return mMapInfo.size();
}

int MapLoader::GetSizeofPlayerData()
{
	return mPlayerInfo.size();
}

MapData MapLoader::GetMapData(int index)
{
	return mMapInfo[index];
}

PlayerData MapLoader::GetPlayerData(int index)
{
	return mPlayerInfo[index];
}
