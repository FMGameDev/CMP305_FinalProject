#include "Emitter.h"

Emitter::Emitter(XMFLOAT3 position)
	: position_(position)
{
	emitter_behaviour_ = EmitterBehaviour::kDefault;
}

Emitter::~Emitter()
{
}

Particle Emitter::dropParticle(Range particleHeightRange)
{
	Particle particle;

	// set position of the particle depending the emitter behaviour
	if (emitter_behaviour_ == EmitterBehaviour::kDefault)
	{
		particle.position = position_;
	}

	// Assign a random height
	particle.height = Utils::GetRandom(particleHeightRange);

	return particle;
}
