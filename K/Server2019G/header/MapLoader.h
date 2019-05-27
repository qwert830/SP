#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

enum MAPLOADERSTATE
{
	MAPINFO, PLAYERINFO
};
struct MapData
{
	float offsetX, offsetY, offsetZ, scalingX, scalingY, scalingZ, rotationY;
	MapData() {}
	MapData(float ox, float oy, float oz, float sx, float sy, float sz, float ry) 
		: offsetX(ox), offsetY(oy), offsetZ(oz), scalingX(sx), scalingY(sy), scalingZ(sz), rotationY(ry) {}
};

struct PlayerData
{
	float tx, ty, tz;
	PlayerData() {}
	PlayerData(float x, float y, float z) : tx(x), ty(y), tz(z) {}
};


class MapLoader
{
public:
	MapLoader();
	~MapLoader();

private:
	void LoadMapData();
	void LoadPlayerData();

public:
	void LoadData();
	std::vector<MapData> mMapInfo;
	std::vector<PlayerData> mPlayerInfo;
};

