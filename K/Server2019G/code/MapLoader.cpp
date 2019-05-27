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
	MapData d;

	std::fstream f;
	f.open("Map.txt");
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
				d.offsetX = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.offsetY = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.offsetZ = (float)atof(temp);
			}
		}
		else if (arr == "s")
		{
			arr = f.get();
			if (arr == "x")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.scalingX = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.scalingY = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.scalingZ = (float)atof(temp);
			}
		}
		else if (arr == "r")
		{
			arr = f.get();
			arr = f.get();
			f.getline(temp, 100, ' ');
			d.rotationY = (float)atof(temp);
			mMapInfo.push_back(d);
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
	PlayerData d;

	std::fstream f;
	f.open("PlayerMap.txt");
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
				d.tx = (float)atof(temp);
			}
			else if (arr == "y")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.ty = (float)atof(temp);
			}
			else if (arr == "z")
			{
				arr = f.get();
				f.getline(temp, 100, ' ');
				d.tz = (float)atof(temp);
				mPlayerInfo.push_back(d);
				i++;
			}
		}
		if (f.eof())
			return;
	}
}