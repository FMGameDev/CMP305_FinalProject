#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "Utils.h"

using namespace DirectX;

enum EmitterBehaviour
{
	kDefault = 0, // by default the position where the particle is initially dropped is the emitter position
	kVolcano = 1 // this emitter will drop particles around its position as a volcano
};

struct Particle
{
	float height = 2.0f;
	XMFLOAT3 position; // position where the particle is dropped (initially)
};

class Emitter
{

public:
	// constructor
	Emitter(XMFLOAT3 position);

	// destructor
	~Emitter();

	// return a particle
	Particle dropParticle(Range particleHeightRange);

private:
	XMFLOAT3 position_; // emitter position
	EmitterBehaviour emitter_behaviour_;
};

