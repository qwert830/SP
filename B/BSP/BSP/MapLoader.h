#pragma once
#include "d3dUtil.h"

struct MapData
{
	MapData() {};
	MapData(float ox, float oy, float oz, float sx, float sy, float sz, float ry) 
		: offsetX(ox), offsetY(oy), offsetZ(oz), scalingX(sx), scalingY(sy), scalingZ(sz), rotationY(ry) {};
	float offsetX, offsetY, offsetZ, scalingX, scalingY, scalingZ, rotationY;
};


class MapLoader
{
public:
	MapLoader();
	~MapLoader();

public:
	void LoadMapData();
	int GetSizeofData();
	MapData GetMapData(int index);

private:
	std::vector<MapData> mMapInfo;
};

