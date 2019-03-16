#pragma once
#include "d3dUtil.h"

struct UVPos
{
	UVPos() {}
	UVPos(float iu, float iv, float iw, float ih) : u(iu), v(iv), w(iw), h(ih) {}

	float u = 0, v = 0, w = 0, h = 0;
};

class FontManager
{
public:
	FontManager();
	~FontManager();

public:
	bool InitFont();
	UVPos GetUV(char id);

private:
	std::unordered_map<char, UVPos> uvInfo;
};

