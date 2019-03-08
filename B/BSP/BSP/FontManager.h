#pragma once
#include "d3dUtil.h"

struct UVPos
{
	UVPos() { u = 0; v = 0; w = 0; h = 0; }
	UVPos(float iu, float iv, float iw, float ih) : u(iu), v(iv), w(iw), h(ih) {}

	float u;
	float v;
	float w;
	float h;
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

