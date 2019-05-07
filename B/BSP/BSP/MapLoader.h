#pragma once
#include "d3dUtil.h"

struct MapData
{
	MapData() {}
	MapData(float ox, float oy, float oz, float sx, float sy, float sz, float ry) 
		: offsetX(ox), offsetY(oy), offsetZ(oz), scalingX(sx), scalingY(sy), scalingZ(sz), rotationY(ry) {}
	float offsetX, offsetY, offsetZ, scalingX, scalingY, scalingZ, rotationY;
};

struct PlayerData
{
	PlayerData() {}
	PlayerData(float x, float y, float z) : tx(x), ty(y), tz(z) {}
	float tx, ty, tz;
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
	int GetSizeofMapData();
	int GetSizeofPlayerData();
	MapData GetMapData(int index);
	PlayerData GetPlayerData(int index);

private:
	std::vector<MapData> mMapInfo;
	std::vector<PlayerData> mPlayerInfo;
};

