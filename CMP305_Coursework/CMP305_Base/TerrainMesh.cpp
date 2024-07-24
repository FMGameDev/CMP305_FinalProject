#include "TerrainMesh.h"

#define _USE_MATH_DEFINES // it has to be set the first thing before any include <>
#include <cmath>

#include <cstdlib>
#include <time.h>       /* time */


TerrainMesh::TerrainMesh( ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMINT2 iResolution )
{
	/* initialize random seed: */
	srand(time(NULL));

	vertexBuffer = nullptr;

	Resize(device, deviceContext, iResolution);

	emitter = new Emitter(GetRandomPos()); // create emitter and set it in a random pos


}

TerrainMesh::~TerrainMesh()
{
	delete[] heightMap;
	heightMap = 0;

	delete emitter;
	emitter = nullptr;

	// Run parent deconstructor
	BaseMesh::~BaseMesh();
}


void TerrainMesh::CreateBuffers(ID3D11Device* device, VertexType* vertices, unsigned long* indices) {

	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	// Set up the description of the dyanmic vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;
	// Now create the vertex buffer.
	device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer);

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;
	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	device->CreateBuffer(&indexBufferDesc, &indexData, &indexBuffer);
}

void TerrainMesh::Resize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, XMINT2 newResolution) {

	// controlif the resolution has actually changed
	if (resolution.x == newResolution.x && resolution.y == newResolution.y)
	{
		return;
	}
	
	// if it has changed then apply it
	resolution = newResolution;

	// remove old heightMap
	delete heightMap;
	heightMap = nullptr;

	// init new heightMap
	heightMap = new float[(resolution.x + 1) * (resolution.y + 1)];
	Flatten();

	// Remove old vertex buffer
	if (vertexBuffer != NULL)
	{
		vertexBuffer->Release();
	}
	vertexBuffer = NULL;

	// Init new buffer
	Regenerate(device, deviceContext);
}

// Build quad (with texture coordinates and normals).
#define cClamp(value, minimum, maximum) (max(min((value),(maximum)),(minimum)))
void TerrainMesh::Regenerate(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;

	float fDepth = static_cast<float>(resolution.x);
	float fWidth = static_cast<float>(resolution.y);

	vertexCount = (resolution.x + 1) * (resolution.y + 1); // need one more row and column if counting per patch
	indexCount = resolution.x * resolution.y * 12; // using 12 for seamless

	vertices = new VertexType[vertexCount];
	indices = new unsigned long[indexCount];

	///////////////// VERTICES //////////////////

	// Load the vertex array with data.
	for (int m = 0; m < (resolution.x + 1); m++) // x
	{
		// calculate the v coord for texture
		float v = static_cast<float>(m) / fDepth;

		for (int n = 0; n < (resolution.y + 1); n++) // y
		{
			// calculate the u coord for texture
			float u = static_cast<float>(n) / fWidth;

			// get vertex index
			int index = GetHeightMapIndex(m, n);
			vertices[index].position = XMFLOAT3((float)n, heightMap[index], (float)m);
			vertices[index].texture = XMFLOAT2(v, u);
			vertices[index].normal = XMFLOAT3(0.0f, 1.0f, 0.0f); // looking up (+y)
		}
	}

	///////////////// NORMALS //////////////////
	//Set up normals
	for (int m = 0; m < resolution.x; m++)
	{
		for (int n = 0; n < resolution.y; n++)
		{
			//Calculate the plane normals
			XMFLOAT3 a, b, c;	//Three corner vertices
			a = vertices[GetHeightMapIndex(m, n)].position;
			b = vertices[GetHeightMapIndex(m, n+1)].position;
			c = vertices[GetHeightMapIndex(m+1, n)].position;

			//Two edges
			XMFLOAT3 ab(c.x - a.x, c.y - a.y, c.z - a.z);
			XMFLOAT3 ac(b.x - a.x, b.y - a.y, b.z - a.z);

			//Calculate the cross product
			XMFLOAT3 cross;
			cross.x = ab.y * ac.z - ab.z * ac.y;
			cross.y = ab.z * ac.x - ab.x * ac.z;
			cross.z = ab.x * ac.y - ab.y * ac.x;
			float mag = (cross.x * cross.x) + (cross.y * cross.y) + (cross.z * cross.z);
			mag = sqrtf(mag);
			cross.x /= mag;
			cross.y /= mag;
			cross.z /= mag;
			vertices[GetHeightMapIndex(m, n)].normal = cross;
		}
	}

	///////////////// INDICES //////////////////
	// Indices Guide for passing 12 control points instead of 4:
	//			   6 -------- 7
	//			   |		  |
	//			   |		  |
	//			   |		  |
	//  8 -------- 0 -------- 3 -------- 5
	//	|		   |		  |			 |
	//	|		   |		  |			 |
	//	|		   |		  |			 |
	//  9 -------- 1 -------- 2 -------- 4
	//			   |		  |
	//			   |		  |
	//			   |		  |
	//			  10 -------- 11
	int i = 0;
	for (int m = 0; m < resolution.x; m++) // x
	{
		for (int n = 0; n < resolution.y; n++) // y
		{
			// from 0 to 3 are the actual quad (the first 4 indices)
			// This indices will be in CCW
			indices[i++] = GetHeightMapIndex(m + 1, n + 0); // 0 - furthest left corner	
			indices[i++] = GetHeightMapIndex(m + 0, n + 0); // 1 - closest left corner
			indices[i++] = GetHeightMapIndex(m + 0, n + 1); // 2 - closest right corner
			indices[i++] = GetHeightMapIndex(m + 1, n + 1); // 3 - fursthest right corner			


			// Neighbours //

			// 4 and 5 are +n
			indices[i++] = GetHeightMapIndex(cClamp(m + 0, 0, resolution.x), cClamp(n + 2, 0, resolution.y)); // 4
			indices[i++] = GetHeightMapIndex(cClamp(m + 1, 0, resolution.x), cClamp(n + 2, 0, resolution.y)); // 5

			// 6 and 7 are +m
			indices[i++] = GetHeightMapIndex(cClamp(m + 2, 0, resolution.x), cClamp(n + 0, 0, resolution.y));  // 6
			indices[i++] = GetHeightMapIndex(cClamp(m + 2, 0, resolution.x), cClamp(n + 1, 0, resolution.y));  // 7

			// 8 and 9 are -n
			indices[i++] = GetHeightMapIndex(cClamp(m + 1, 0, resolution.x), cClamp(n - 1, 0, resolution.y)); // 8
			indices[i++] = GetHeightMapIndex(cClamp(m + 0, 0, resolution.x), cClamp(n - 1, 0, resolution.y)); // 9

			//10 and 11 are -m
			indices[i++] = GetHeightMapIndex(cClamp(m - 1, 0, resolution.x), cClamp(n + 0, 0, resolution.y)); // 10
			indices[i++] = GetHeightMapIndex(cClamp(m - 1, 0, resolution.x), cClamp(n + 1, 0, resolution.y)); // 11
		}
	}

	//If we've not yet created our dyanmic Vertex and Index buffers, do that now
	if (vertexBuffer == nullptr)
	{
		CreateBuffers(device, vertices, indices);
	}
	else 
	{
		//If we've already made our buffers, update the information
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

		//  Disable GPU access to the vertex buffer data.
		deviceContext->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		//  Update the vertex buffer here.
		memcpy(mappedResource.pData, vertices, sizeof(VertexType) * vertexCount);
		//  Reenable GPU access to the vertex buffer data.
		deviceContext->Unmap(vertexBuffer, 0);
	}

	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;
	delete[] indices;
	indices = 0;
}


void TerrainMesh::sendData(ID3D11DeviceContext* deviceContext, D3D_PRIMITIVE_TOPOLOGY top)
{
	unsigned int stride;
	unsigned int offset;

	stride = sizeof(VertexType);
	offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	// Set the type of primitive that should be rendered from this vertex buffer, in this case control patch for tessellation.
	deviceContext->IASetPrimitiveTopology(top);
}


//////////////////////////////////////////////////////////////// TERRAIN MANIPULATION HEIGHT MAP FUNCTIONS ////////////////////////////////////////////////////////////////



//////////////////////////////// BUILD HEIGHT MAP FROM 0 FUNCTIONS ////////////////////////////////

void TerrainMesh::BuildSinCosWavesHeightMap(WavesData* wavesData, float dt) {

	float height = 0.0f;

	//Scale everything so that the look is consistent across terrain resolutions
	const float scaleM = terrainSize / (float)resolution.x;
	const float scaleN = terrainSize / (float)resolution.y;

	for (int m = 0; m < resolution.x + 1; m++)
	{
		for (int n = 0; n < resolution.y + 1; n++)
		{
			// Waves along x-axis
			height = (sin((float)n * wavesData->frequency.x * scaleN + wavesData->offset.x)) * wavesData->amplitude.x; // Waves 1 On x
			height += (sin((float)n * wavesData->frequency.x * 2.0f * scaleN + wavesData->offset.x)) * wavesData->amplitude.x * 0.5f; // Waves 2 On x (it is half of amplitud1 and double of frequency1)
			height += (sin((float)n * wavesData->frequency.x * 4.0f * scaleN + wavesData->offset.x)) * wavesData->amplitude.x * 0.25f; // Waves 3 On x (it is half of amplitud2 and double of frequency2)
			// Waves along z-axis
			height += (cos((float)m * wavesData->frequency.z * scaleM + wavesData->offset.z)) * wavesData->amplitude.z; // Waves 1 On z
			height += (cos((float)m * wavesData->frequency.z * 2.0f * scaleM + wavesData->offset.z)) * wavesData->amplitude.z * 0.5f; // Waves 2 On z
			height += (cos((float)m * wavesData->frequency.z * 4.0f * scaleM + wavesData->offset.z)) * wavesData->amplitude.z * 0.25f; // Waves 3 On z
			heightMap[GetHeightMapIndex(m, n)] = height;
		}
	}

	// Apply offset for the next pass if the user wants the waves to be moved
	if (wavesData->moveWaves[0]) // x
		wavesData->offset.x += dt; // moving the wave in x
	if (wavesData->moveWaves[2]) // z
		wavesData->offset.z += dt; // moving the wave in z
}

void TerrainMesh::BuildRandomHeightMap(Range heightRange)
{
	// The number inside the sin or cos modify the amplitude and the number outside is the Amplitude
	for (int m = 0; m < resolution.x + 1; m++)
	{
		for (int n = 0; n < resolution.y + 1; n++)
		{
			heightMap[GetHeightMapIndex(m, n)] = Utils::GetRandom(heightRange); // random number in the range [min, max]
		}
	}
}


//////////////////////////////// MODIFY HEIGHT MAP FUNCTIONS ////////////////////////////////

void TerrainMesh::Flatten()
{
	for (int m = 0; m < (resolution.x + 1); m++)
	{
		for (int n = 0; n < (resolution.y + 1); n++)
		{
			heightMap[GetHeightMapIndex(m, n)] = 0.0f;
		}
	}
}

void TerrainMesh::Fault(Range heightOffsetRange)
{
	XMFLOAT3 point1, point2, currentVertex;
	XMVECTOR faultLine, linkedLine, crossResult;

	point1 = XMFLOAT3(rand() % resolution.x+1, 0.0f, rand() % resolution.y+1); // a random point in the map
	point2 = XMFLOAT3(point1.x, point1.y, point1.z + 1.0f); // point 2 = point 1 displaced in z-axis
	faultLine = XMVectorSet(point2.x - point1.x, point2.y - point1.y, point2.z - point1.z, 1.0f); // Line from point 1 to point 2
	faultLine = XMVector3Rotate(faultLine, XMQuaternionRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), ((rand() % 360) * M_PI) / 180)); // Rotate that line randomly

	// get the offset to move up and move down
	float heightOffset = Utils::GetRandom(heightOffsetRange);

	for (int m = 0; m < resolution.x + 1; m++)
	{
		for (int n = 0; n < resolution.y + 1; n++)
		{
			currentVertex = XMFLOAT3(m, 0.0f, n); // get the current vertex
			linkedLine = XMVectorSet(currentVertex.x - point1.x, currentVertex.y - point1.y, currentVertex.z - point1.z, 1.0f); // line from the current vertex to the a point in the fault line (point 1) 

			crossResult = XMVector3Cross(faultLine, linkedLine);

			// detect if the current point is in the left or right side of the fault line
			if (XMVectorGetY(crossResult) > 0) // left side
			{
				// move up
				heightMap[GetHeightMapIndex(m, n)] += heightOffset;
			}
			else // right side
			{
				// move down
				heightMap[GetHeightMapIndex(m, n)] -= heightOffset;
			}
		}
	}
}

void TerrainMesh::Smooth()
{
	float* smoothedHeightMap = new float[(resolution.x + 1) * (resolution.y + 1)];

	for (int m = 0; m < resolution.x+1; m++)
	{
		for (int n = 0; n < resolution.y+1; n++)
		{
			smoothedHeightMap[GetHeightMapIndex(m, n)] = NeighboursAverage(m, n);;
		}
	}

	// replace the old height map with the filtered one
	heightMap = smoothedHeightMap;
}

void TerrainMesh::ParticleDeposition(Range heightRange)
{
	// call to the emitter to drop a particle
	Particle particle = emitter->dropParticle(heightRange);

	// get x,z position of the particle
	int x = (int)particle.position.x;
	int z = (int)particle.position.z;

	XMFLOAT3 lowest_height_point = particle.position;
	bool found = false;

	// look throught the possible neighbours of the current point
	for (int m = x - 1; m <= x + 1; m++)
	{
		for (int n = z - 1; n <= z + 1; n++)
		{
			if (InBounds(m, n) && heightMap[GetHeightMapIndex(m, n)] < heightMap[GetHeightMapIndex(lowest_height_point.x, lowest_height_point.z)]) // avoid to check the center cell again and check if it is in bounds
			{
				lowest_height_point = XMFLOAT3(m, 0.0f, n);// save the new lowest height point
			}
		}
	}

	// add height to the map
	heightMap[GetHeightMapIndex(lowest_height_point.x, lowest_height_point.z)] += particle.height;
}

void TerrainMesh::AntiParticleDeposition(Range heightRange)
{
	// call to the emitter to drop a particle
	Particle particle = emitter->dropParticle(heightRange);

	// get x,z position of the particle
	int x = (int)particle.position.x;
	int z = (int)particle.position.z;

	XMFLOAT3 highest_height_point = particle.position;
	bool found = false;

	// look throught the possible neighbours of the current point
	for (int m = x - 1; m <= x + 1; m++)
	{
		for (int n = z - 1; n <= z + 1; n++)
		{
			if (InBounds(n, m) && heightMap[GetHeightMapIndex(m, n)] > heightMap[GetHeightMapIndex(highest_height_point.x, highest_height_point.z)]) // avoid to check the center cell again and check if it is in bounds
			{
				highest_height_point = XMFLOAT3(m, 0.0f, n);// save the new lowest height point
			}
		}
	}

	// substract height to the map
	heightMap[GetHeightMapIndex(highest_height_point.x, highest_height_point.z)] -= particle.height;
}



void TerrainMesh::DiamondSquareAlgorithm(Range heightOffsetRange)
{
	// Check if this algorithm can be applied to this terrain 
	// The vertices needs to be (2^n)+1 where n>0
	// By taking log2 of N and then pass it to floor and ceil if both gives same result then N is power of 2
	// as we are checking 2^n+1 then we neeed to substract 1 from the resolution to check this
	// Removing the posibility of (2^0)+1 => 1+1 => 2
	if ((ceil(log2(resolution.x)) != floor(log2(resolution.x)) || resolution.x == 2) ||
		(ceil(log2(resolution.y)) != floor(log2(resolution.y)) || resolution.y == 2))
		
	{
		return; // exit this function as the terrain resolution is odd
	}

	// Initialise corners points [(m*n) localitation in the array heightMap] //
	int topLeft, topRight, bottomLeft, bottomRight;
	int center;

	// set m and n values
	int m_start = 0;
	int m_end = resolution.x;
	int n_start = 0;
	int n_end = resolution.y;

	// get the height map indices of the corners in the heightmap array
	topLeft = GetHeightMapIndex(m_start, n_start);
	topRight = GetHeightMapIndex(m_start, n_end);
	bottomLeft = GetHeightMapIndex(m_end, n_start);
	bottomRight = GetHeightMapIndex(m_end, n_end);

	// set the height offset for the initial corner points
	Range tmpHeightOffsetRange = heightOffsetRange;

	// Asign a random height to each corner
	heightMap[topLeft] = Utils::GetRandom(tmpHeightOffsetRange);
	heightMap[topRight] = Utils::GetRandom(tmpHeightOffsetRange);
	heightMap[bottomLeft] = Utils::GetRandom(tmpHeightOffsetRange);
	heightMap[bottomRight] = Utils::GetRandom(tmpHeightOffsetRange);

	XMINT2 chunkSize =  XMINT2(resolution.x, resolution.y); // portion we are working on

	while (chunkSize.x > 1 && chunkSize.y > 1)
	{
		// get the half of the portion we are working on
		XMINT2 half = XMINT2(chunkSize.x / 2, chunkSize.y / 2);

		// Apply Square Step //
		SquareStep(m_start, m_end, n_start, n_end, chunkSize, tmpHeightOffsetRange, half);

		// Apply Diamond Step //
		DiamondStep(m_start, m_end, n_start, n_end, chunkSize, tmpHeightOffsetRange, half);

		// halve the portion of the plane where to work next
		chunkSize = XMINT2(chunkSize.x/2.0f, chunkSize.y/2.0f);

		// halve the height offset
		tmpHeightOffsetRange.min /= 2.0f;
		tmpHeightOffsetRange.max /= 2.0f;
	}
}


void TerrainMesh::SquareStep(int& m_start, int& m_end, int& n_start, int& n_end, XMINT2& chunkSize, Range& tmpHeightOffsetRange, XMINT2& half)
{
	for (int m = m_start; m < m_end; m += chunkSize.x)
	{
		for (int n = n_start; n < n_end; n += chunkSize.y)
		{
			// get the height map indices of the corners of the square in the heightmap array
			int topLeft = GetHeightMapIndex(m, n);
			int topRight = GetHeightMapIndex(m, n + chunkSize.y);
			int bottomLeft = GetHeightMapIndex(m + chunkSize.x, n);
			int bottomRight = GetHeightMapIndex(m + chunkSize.x, n + chunkSize.y);

			// calculate the average of the four corners
			float cornersAvg = (heightMap[topLeft] + heightMap[topRight] + heightMap[bottomLeft] + heightMap[bottomRight]) / 4.0f;

			// square centre point
			int centre = GetHeightMapIndex(m + half.x, n + half.y);

			// set the height to the centre point
			float randomHeightOffset = Utils::GetRandom(tmpHeightOffsetRange);
			heightMap[centre] = cornersAvg + randomHeightOffset;
		}
	}
}

void TerrainMesh::DiamondStep(int& m_start, int& m_end, int& n_start, int& n_end, XMINT2& chunkSize, Range& tmpHeightOffsetRange, XMINT2& half)
{
	for (int m = m_start; m <= m_end; m += half.x)
	{
		for (int n = (m + half.x) % chunkSize.y; n <= n_end; n += chunkSize.y)
		{
			int count = 0;
			float cornersSum = 0;

			// top corner
			if (InBounds(m - half.x, n))
			{
				cornersSum += heightMap[GetHeightMapIndex(m - half.x, n)];
				count++;
			}
			// left corner
			if (InBounds(m, n - half.y))
			{
				cornersSum += heightMap[GetHeightMapIndex(m, n - half.y)];
				count++;
			}
			// right corner
			if (InBounds(m, n + half.y))
			{
				cornersSum += heightMap[GetHeightMapIndex(m, n + half.y)];
				count++;
			}
			// bottom corner
			if (InBounds(m + half.x, n))
			{
				cornersSum += heightMap[GetHeightMapIndex(m + half.y, n)];
				count++;
			}

			// calculate average
			float cornersAvg = (float)cornersSum / (float)count;

			// diamond center point
			int center = GetHeightMapIndex(m, n);

			// set the value to the center point of the diamond
			float random = Utils::GetRandom(tmpHeightOffsetRange);
			heightMap[center] = cornersAvg + random;
		}
	}
}



//////////////////////////////// TOOL FUNCTIONS FOR HEIGHT MAP MANIPULATION ////////////////////////////////

XMFLOAT3 TerrainMesh::GetRandomPos()
{
	return XMFLOAT3(Utils::GetRandom(0.0f, (float)resolution.x), 0.0f, Utils::GetRandom(0.0f, (float)resolution.y));
}

float TerrainMesh::NeighboursAverage(int mPos, int nPos) // neighbour postion ( mPpos, nPos)
{
	// Function computes the average height of the ik element.
	// It averages itself with its eight neighbour pixels.
	// Note that if a pixel is missing neighbour, we just don't include it
	// in the average--that is, edge pixel don't have a neighbour pixel.
	//      
	// ---------
	// | 1| 2| 3|
	// | 4|mn| 6|
	// | 7| 8| 9|
	// ---------

	float totalHeight = 0.0f; // sum of the neighbours height plus the current point itself
	int numPoints = 0; // number of points

	// look throught the possible neighbours of the current point
	for (int m = mPos - 1; m <= mPos + 1; m++)
	{
		for (int n = nPos - 1; n <= nPos + 1; n++)
		{
			if (InBounds(m, n))
			{
				totalHeight += heightMap[GetHeightMapIndex(m, n)];
				numPoints++; // count this point
			}
		}
	}

	return totalHeight / (float)numPoints; // return the average
}


bool TerrainMesh::InBounds(int m, int n)
{
	// True if ik are valid indices; false otherwise
	return (m >= 0 && m <= resolution.x && n >= 0 && n <= resolution.y);
}

int TerrainMesh::GetHeightMapIndex(int m, int n)
{
	// m == rows == x
	// n == columns == y
	return (n + (m * (resolution.y + 1)));
}
