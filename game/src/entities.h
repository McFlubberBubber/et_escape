#ifndef ENTITIES_H
#define ENTITIES_H

#include "raylib.h"
#include "base.h"

enum Pickup_Type {
	PICKUP_UNINITIALIZED,

	PICKUP_COIN,
	PICKUP_SHIELD,
	PICKUP_DOUBLE_SHOT,
};

enum Entity_Type {
	ENTITY_UNINITIALIZED = 0,

	ENTITY_PLAYER,
	ENTITY_INVADER, // Alien / Enemy.
	ENTITY_PICKUP,

	ENTITY_PLAYER_MISSILE,
	ENTITY_INVADER_MISSILE,
};

struct Entity_Handle {
	s32 id;  // Basically an index into the entities array.
	s32 gen; // Unique identifier for each entity.
};

// We bundle literally everything into this struct rather than making seperate
// structs for the Player, Invader, Missile, etc.
struct Entity {
	Vector2 pos;
	Vector2 vel;

	Texture *map;
	
	Entity_Type type;
	Pickup_Type pickup_type;

	// @TODO: Currently, the player can only have one powerup at a time.
	Pickup_Type current_pickup;
	
	// This field is used for different things based on the entity's context.
	// Therefore it really isn't explicit what this field does so I am going to
	// list the stuff that it is used for here, as a comment:
	//
	// - Expiration timer for the player's missiles.
	// - Randomized fire cooldown that each invader spawns with.
	// - Powerup duration tracker for the player's current powerup.
	float duration;

	s32 trail_emitter_index = -1; // This is here only for missiles to own the emitter.

	bool is_dead;
	s32 gen;
};

#endif
