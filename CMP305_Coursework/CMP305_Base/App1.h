// Application.h
#ifndef _APP1_H
#define _APP1_H

// Includes
#include "DXF.h"	// include dxframework
#include "TessellationShader.h"
#include "TerrainMesh.h"
#include <vector>


class App1 : public BaseApplication
{
public:

	App1();
	~App1();
	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in, bool VSYNC, bool FULL_SCREEN);

	bool frame();

protected:
	bool update();
	bool render();
	void gui();

private:
	TessellationShader* tessellationShader;
	TerrainMesh* m_Terrain;

	Light* light;

	// for plane
	float dMinMax[2] = { 1.0f, 47.0f };
	float lvlOfDetail[2] = { 1.0f, 64.0f };
	int provisionalResolution[2]; // resolution to show on IMGUI

	// variable to create sin/cos waves in the terrain
	WavesData wavesData;
	bool renderFrameWaves; // to render waves which are moving in each frame

	// variable to control at what height the textures are set as maximum
	std::vector<float> maxTextureHeight = { 0.0f, 5.0f, 15.0f, 25.0f };
	bool fakeWaterLvlToggle;

	// Max and min values which will be used for getting a random height for the different procedural methods
	Range randomMinMaxHeight;
	Range diamondSquareHeightOffsetRange;
	Range faultHeightRange;
	Range particleDepoHeightRange;
};

#endif