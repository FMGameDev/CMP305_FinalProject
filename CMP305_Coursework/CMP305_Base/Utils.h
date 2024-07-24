#pragma once
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Range
{
	Range()
	{
		min = 0.0f;
		max = 10.0f;
	}

	float min;
	float max;
};

class Utils
{
public:
	// Return a random float using a range structure as parameter
	static float GetRandom(Range range);

	// Return a random float using two floats as parameters
	// Function from: https://www.delftstack.com/howto/cpp/how-to-generate-random-float-number-in-cpp/
	static float GetRandom(float from, float to);
};