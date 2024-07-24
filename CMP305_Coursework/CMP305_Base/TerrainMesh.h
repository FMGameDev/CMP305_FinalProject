#pragma once
#include "BaseMesh.h"

#include "Emitter.h"
#include "Utils.h"
#include "SimplexNoise.h"

// Frecuency, amplitude and all the data for Waves
struct WavesData
{
	WavesData()
	{ 
		// initialise values
		frequency = XMFLOAT3(0.156f, 0.0f, 0.346f);
		amplitude = XMFLOAT3(2.602f, 0.0f, 4.065f);
		offset = XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	XMFLOAT3 frequency;
	XMFLOAT3 amplitude;
	XMFLOAT3 offset;
	bool moveWaves[3] = { false, false, false }; // {x, y, z}
};

class TerrainMesh : public BaseMesh {

public:
	// Constructor Class
	TerrainMesh( ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMINT2 iResolution = XMINT2(128,128));
	// Destructor class:
	// - Cleanup the heightMap
	// - Remove all the pointers created in this function
	~TerrainMesh();

	// Override sendData() to change topology type. Control point patch list is required for tessellation
	void sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top = D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST) override;


	// Change the size of the terrain
	void Resize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMINT2 newResolution);

	// Set up the heightmap and create or update the appropriate buffers
	void Regenerate(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

	// Get the resolution of the terrain (The number of unit quad on x-axis and z-axis subtracting One)
	XMINT2 GetResolution()const { return resolution; }

	//// TERRAIN MANIPULATION HEIGHT MAP FUNCTIONS //// 

	// BUILD HEIGHT MAP FROM 0 FUNCTIONS //
	// 
	// Filling an array of floats that represent the height values at each grid point.
	// By producing a Sine a Cosene wave along the X-axis and Z-axis
	void BuildSinCosWavesHeightMap(WavesData* wavesData, float dt = 0.0f);
	// Filling an array of floats that represent the height values at each grid point.
	// By using random numbers in the height offset range
	void BuildRandomHeightMap(Range heightRange);

	// MODIFY HEIGHT MAP FUNCTIONS //
	// Set to 0 the height of every point
	void Flatten();
	// Fault is made by adding or subtracting the max height value
	void Fault(Range heightOffsetRange);
	// Algorithm from 3D Game Programming with Directx11 by Frank D. Luna (Page 603)
	// Smooth all the terrain
	void Smooth();
	// It randomly distributes, or emits, particles across the surface of our terrain.
	// Each time a particle "lands", raise the terrain a little
	// As the particles stack up, you get natural raises in the terrain and organic features
	// if there is a lower point to the left, right, up, down to the particle deposition then it is placed there.
	void ParticleDeposition(Range heightRange);
	// Susbtract height to the highest point surrounding the particle position
	void AntiParticleDeposition(Range heightRange);
	// Apply the Diando-Square (Midpoint Displacement) Algorithm to the terrain
	// It has been based on the pseudocode: https://www.youtube.com/watch?v=4GuAV1PnurU&t=796s
	void DiamondSquareAlgorithm(Range heightRange);

private:
	//Create the vertex and index buffers that will be passed along to the graphics card for rendering
	//For CMP305, you don't need to worry so much about how or why yet, but notice the Vertex buffer is DYNAMIC here as we are changing the values often
	void initBuffers(ID3D11Device*) override {};
	void CreateBuffers( ID3D11Device* device, VertexType* vertices, unsigned long* indices );
	int GetHeightMapIndex(int m, int n); // return the height map index

	// check if a point is in the map/terrain
	bool InBounds(int m, int n);
	// return the height average of the neighbours to that point (inluding that point too)
	float NeighboursAverage(int m, int n);
	// return a random position from the map
	XMFLOAT3 GetRandomPos();

	void SquareStep(int& m_start, int& m_end, int& n_start, int& n_end, XMINT2& chunkSize, Range& tmpHeightOffsetRange, XMINT2& half);
	void DiamondStep(int& m_start, int& m_end, int& n_start, int& n_end, XMINT2& chunkSize, Range& tmpHeightOffsetRange, XMINT2& half);

	const float m_UVscale = 10.0f;			//Tile the UV map 10 times across the plane
	const float terrainSize = 100.0f;		//What is the width and height of our terrain
	// resolution per patch, per vertex is resolution+1
	XMINT2 resolution; // x=m=rows, y=n=columns
	float* heightMap;

	// Object which will randomly emit particles across the terrain
	Emitter* emitter;


};
