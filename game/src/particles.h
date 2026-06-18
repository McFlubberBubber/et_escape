#ifndef PARTICLES_H
#define PARTICLES_H

#include "base.h"
#include "raylib.h"

const u32 MAX_PARTICLES_PER_EMITTER = 128;

enum Emitter_Type : u8 {
	EMITTER_NONE,
	
	EMITTER_EXPLODE,

	EMITTER_PLAYER_MISSILE_TRAIL,
	EMITTER_INVADER_MISSILE_TRAIL
};

struct Particle {
	Vector2 pos;
	Vector2 vel;

	Color color;

	float elapsed;	// Time since spawn.
	float lifetime; // Total lifespan.

	float size;

	bool is_active;
};

struct Particle_Emitter {
	Particle particles[MAX_PARTICLES_PER_EMITTER];

	Vector2 pos;
	Vector2 dir;

	float min_lifetime;
	float max_lifetime;

	float min_size = 0;
	float max_size;

	Color color_start;
	Color color_end;

	Emitter_Type type;
	bool is_dead;
};

#endif
