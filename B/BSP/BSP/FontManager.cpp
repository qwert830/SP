#include <fstream>
#include <iostream>
#include <string>
#include "FontManager.h"

using namespace std;

FontManager::FontManager()
{
}


FontManager::~FontManager()
{
}

bool FontManager::InitFont()
{
	string arr;
	char temp[100];
	char id[100];
	float x[100];
	float y[100];
	float width[100];
	float height[100];

	fstream f;
	f.open("Resource/font.fnt");
	if (!f.is_open())
		return false;
	int i = 0;
	while (1)
	{
		arr = f.get();
		cout << arr;
		if (arr == "d")
		{
			arr = f.get();
			if (arr == "=")
			{
				f.getline(temp, 100, ' ');
				id[i] = atoi(temp);
			}
		}
		else if (arr == "x")
		{
			arr = f.get();
			if (arr == "=")
			{
				f.getline(temp, 100, ' ');
				x[i] = atoi(temp);
			}
		}
		else if (arr == "y")
		{
			arr = f.get();
			if (arr == "=")
			{
				f.getline(temp, 100, ' ');
				y[i] = atoi(temp);
			}
		}
		else if (arr == "h")
		{
			arr = f.get();
			if (arr == "=")
			{
				f.getline(temp, 100, ' ');
				width[i] = atoi(temp);
			}
			else if (arr == "t")
			{
				arr = f.get();
				if (arr == "=")
				{
					f.getline(temp, 100, ' ');
					height[i] = atoi(temp);
					uvInfo[id[i++]] = UVPos{ x[i] / 512.0f,y[i] / 512.0f ,width[i] / 512.0f ,height[i] / 512.0f };
				}
			}
		}
		
		if (f.eof())
			break;
	}
	return true;
}

UVPos FontManager::GetUV(char id)
{
	UVPos temp;
	temp = uvInfo[id];

	return temp;
}
