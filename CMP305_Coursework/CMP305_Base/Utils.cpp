#include "Utils.h"

#include <cstdlib>
#include <ctime>

#include <iostream>
#include <random>
#include <iomanip>

using std::cout;
using std::endl;
using std::setprecision;


float Utils::GetRandom(Range range)
{
	return Utils::GetRandom(range.min, range.max);
}

float Utils::GetRandom(float from, float to)
{
	float min, max;
	float randomNum;

	if (from < to)
	{
		min = from;
		max = to;
	}
	else
	{
		min = to;
		max = from;
	}

	std::random_device rd;
	std::default_random_engine eng(rd());
	std::uniform_real_distribution<float> distr(min, max);

	randomNum = distr(eng);
	cout << "Random Num:" << randomNum << "\n";

	return randomNum;
}


