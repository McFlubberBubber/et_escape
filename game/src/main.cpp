#include "raylib.h"
#include "raymath.h"

#include "entities.h"
#include "particles.h"

float dt; // Deltatime.
float time_until_game_close = 3;

const u32 level_cap = 10;

constexpr float ASPECT_RATIO = (224.0f / 288.0f);
constexpr u32 VPIXELS = 960;

const u32 window_width  = static_cast<u32>(VPIXELS * ASPECT_RATIO); 
const u32 window_height = VPIXELS;

const u32 center_width  = static_cast<u32>(window_width  * 0.5f);
const u32 center_height = static_cast<u32>(window_height * 0.5f); 

const u32 desired_fps = 60;

const char *data_path;
const char *app_path;
char data_path_buffer[256];

const u32 player_width  = 50;
const u32 player_height = 50;

const u32 invader_width  = 60;
const u32 invader_height = 50;

const u32 player_missile_width  = 3;
const u32 player_missile_height = 25;

const u32 invader_missile_width  = 5;
const u32 invader_missile_height = 25;

const u32 pickup_width  = 25;
const u32 pickup_height = 25;

Image game_icon;

Font the_font;

Sound sound_player_shoot;
Sound sound_player_dies;
Sound sound_enemy_dies;
Sound sound_pickup_coin;
Sound sound_pickup_power;

Texture2D texture_missing;
Texture2D texture_player_ship;
Texture2D texture_enemy_ship;
Texture2D texture_coin;
Texture2D texture_missile;
Texture2D texture_logo;
Texture2D texture_background;

enum Menu_Item : u8 {
	MENU_START_GAME = 0,
	MENU_QUIT_GAME,

	MENU_ITEM_COUNT
};

enum Program_Mode : u8 {
	PROGRAM_MENU = 0,
	PROGRAM_GAME,

	PROGRAM_MODE_COUNT
};

struct Game_State {
	s32 current_level;
	s32 current_score;
	s32 current_invader_count;

	bool is_paused;
	bool is_over;
	bool draw_hitboxes;

	Program_Mode current_mode;
	Menu_Item    current_menu_item;
};

Game_State the_game; 
bool app_should_close;

Auto_Array<Entity> entities;
Entity_Handle player_handle;

Auto_Array<Particle_Emitter> emitters;

// Wrapper functions around raylib's procedures that automatically use our
// set data_path, so we can just provide the filename directly.
static Image     load_image(const char *filename);
static Font      load_font(const char *filename);
static Sound     load_sound(const char *filename);
static Texture2D load_texture(const char *filename);

static void draw_texture_tiled(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, float scale, Color tint);

static void init_game(Game_State *gs);
static void process_input();
static void update_app();
static void draw_app();

// An Entity_Handle is composed of 2 things, an id (which is used as an index
// into the entities array), and a gen (which is a unique identifier that is
// assigned to each entity upon creation). 
static Entity_Handle add_entity(Entity e);
static void      destroy_entity(Entity_Handle handle);
static Entity       *get_entity(Entity_Handle handle);

static void spawn_invaders();
static void check_level_changes();
static void fire_player_missile();
static void fire_invader_missile(Vector2 pos);
static void spawn_random_pickup(Vector2 pos);

static Particle_Emitter make_exploding_emitter(Vector2 pos, Color color_start, Color color_end);
static Particle_Emitter make_trailing_emitter_for_missile(Vector2 pos, Entity_Type type);

static void emitter_explode(Particle_Emitter *emitter, s32 burst_count);

static s32  add_emitter(Particle_Emitter emitter);
static void spawn_explode_particle(Particle_Emitter *emitter);
static void spawn_trail_particle(Particle_Emitter *emitter);
static void update_each_emitters_particles();
static void draw_emitters();

int main() {
	//
	// Window initialization.
	//
// SetConfigFlags(FLAG_WINDOW_RESIZABLE); // We won't make the window resizable for now.
	InitWindow(window_width, window_height, "E.T Escape");
	SetTargetFPS(desired_fps);
	SetExitKey(KEY_Q);
	app_should_close = false;

	//
	// Asset initialization (loading fonts, audio and textures using the
	// application + data paths).
	//
	app_path = GetApplicationDirectory();

	// We define the game's data path in the build system because we copy the
	// data directory into the build for release builds. For debug builds, we
	// directly path to the data directory that doesn't get exported. So you
	// can sorta treat the define beneath as a check for if we are in debug.
#ifdef GAME_DATA_PATH
	data_path = GAME_DATA_PATH;
#else
	snprintf(data_path_buffer, sizeof(data_path_buffer), "%s%s", app_path, "data/");
	data_path = data_path_buffer;
#endif
	
	printf("app_path:  %s\n", app_path);
	printf("data_path: %s\n", data_path);

	game_icon = load_image("player_ship.png");
	SetWindowIcon(game_icon);

	// @TODO: We should get a cooler font, and assign this current one
	// as the debug font.
	the_font = load_font("Consolas-Regular.ttf");

	InitAudioDevice();
	sound_player_shoot = load_sound("player_shoot.wav");
	sound_player_dies  = load_sound("player_dies.wav");
	sound_enemy_dies   = load_sound("enemy_dies.wav");
	sound_pickup_coin  = load_sound("pickup_coin.wav");
	sound_pickup_power = load_sound("pickup_power.wav");

	SetSoundVolume(sound_enemy_dies, 0.5f);
	SetSoundVolume(sound_player_dies, 0.4f);
	
	texture_missing     = load_texture("missing_texture.png");
    texture_player_ship = load_texture("player_ship.png");
	texture_enemy_ship  = load_texture("alien_ship.png");
	texture_coin        = load_texture("coin.png");
	texture_missile     = load_texture("missile.png");
	texture_logo        = load_texture("logo.png");
	texture_background  = load_texture("background.png");

	SetTextureWrap(texture_background, TEXTURE_WRAP_REPEAT);
	SetTextureFilter(texture_background, TEXTURE_FILTER_BILINEAR);

	//
	// Game state initialization (setting default states and so on).
	//
	init_game(&the_game);

	//
	// Main game loop.
	//
	while (!WindowShouldClose() && !app_should_close) {
		dt = GetFrameTime();
		dt = Clamp(dt, 0.0f, (1.0f / 30.0f));

		process_input();
		update_app();
		draw_app();
	}

	CloseAudioDevice();
	CloseWindow();
	
	return 0;
}

static Image load_image(const char *filename) {
	char path[256];
	snprintf(path, sizeof(path), "%s%s%s", data_path, "textures/", filename);
	return LoadImage(path);
}

static Font load_font(const char *filename) {
	char path[256];
	snprintf(path, sizeof(path), "%s%s%s", data_path, "fonts/", filename);
	return LoadFont(path);
}

static Sound load_sound(const char *filename) {
	char path[256];
	snprintf(path, sizeof(path), "%s%s%s", data_path, "audio/", filename);
	return LoadSound(path);
}

static Texture2D load_texture(const char *filename) {
	char path[256];
	snprintf(path, sizeof(path), "%s%s%s", data_path, "textures/", filename);
	return LoadTexture(path);
}

static void init_game(Game_State *gs) {
	const u32 initial_entities = 64;
	const u32 initial_emitters = 64;

	gs->current_level = 0;
	gs->current_score = 0;
	gs->current_mode = PROGRAM_MENU;
	gs->current_invader_count = 0;

	gs->is_paused = false;
	gs->is_over = false;
	gs->draw_hitboxes = false;

	array_init(&entities, initial_entities);
	array_init(&emitters, initial_emitters);
}

static void reset_game(Game_State *gs) { // This is called even for startup.
	the_game.current_level = 0;
	the_game.current_score = 0;
	the_game.current_invader_count = 0;
	the_game.is_over = false;

	s32 entity_index = 0;
	if (entities.count > 0) {
		For (entities)  {
			Entity_Handle handle = {entity_index, it->gen};
			destroy_entity(handle);
			entity_index += 1;
		}
	}

	if (emitters.count > 0) {
		For (emitters) {
			it->is_dead = true;	
		}
	}

	Entity player = {};
	player.pos.x = center_width - (player_width * 0.5f);
	player.pos.y = window_height * 0.9f;
	player.map = &texture_player_ship;
	player.type = ENTITY_PLAYER;
	player.is_dead = false;
	
	player_handle = add_entity(player);
}

static void handle_menu_inputs() {
	s32 current_item, total_items, prev_item;
	current_item = static_cast<s32>(the_game.current_menu_item);
	total_items  = static_cast<s32>(MENU_ITEM_COUNT);
	prev_item = (current_item - 1 + total_items) % total_items;

	if (IsKeyPressed(KEY_UP))   the_game.current_menu_item = static_cast<Menu_Item>((current_item + 1) % total_items);
	if (IsKeyPressed(KEY_DOWN)) the_game.current_menu_item = static_cast<Menu_Item>(prev_item);

	if (IsKeyPressed(KEY_ENTER)) {
		switch(the_game.current_menu_item) {
		case MENU_START_GAME: {
			if (the_game.is_paused) {
				the_game.is_paused = false;
			} else {
				the_game.current_mode = PROGRAM_GAME;
				the_game.current_menu_item = MENU_START_GAME;
				reset_game(&the_game);
			}
		} break;

		case MENU_QUIT_GAME: { // This case is also used for 'back to main menu'.
			if (the_game.is_paused) {
				the_game.current_mode = PROGRAM_MENU;
				the_game.current_menu_item = MENU_START_GAME;
				the_game.is_paused = false;
				the_game.current_level = 0;
			} else {
				app_should_close = true;
			}
		} break;
		}
	}
}

const float player_movement_speed = 600;

const float pickup_movement_speed = 200;

const float invader_movement_speed_min = 150;
const float invader_movement_speed_max = 250;

static void handle_game_inputs() {
	if (IsKeyPressed(KEY_ESCAPE))  the_game.is_paused = !the_game.is_paused;

	if (the_game.is_paused) {  // We treat inputs for the menu and pause the same.
		handle_menu_inputs();
	} else {                   // Inputs for player control.
		if (IsKeyPressed(KEY_R))  reset_game(&the_game);

		if (the_game.is_over) return;
		Entity *player = get_entity(player_handle);
		Vector2 dir = {};

		if (IsKeyDown(KEY_A))  dir.x -= 1.0f;
		if (IsKeyDown(KEY_D))  dir.x += 1.0f;
		if (IsKeyDown(KEY_W))  dir.y -= 1.0f;
		if (IsKeyDown(KEY_S))  dir.y += 1.0f;

		if (dir.x != 0 || dir.y != 0) {
			float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
			dir.x /= len;
			dir.y /= len;
		}
		player->vel.x = dir.x * player_movement_speed;
		player->vel.y = dir.y * player_movement_speed;

		if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP))  fire_player_missile();
	}
}

static void process_input() {
	if (IsKeyPressed(KEY_E)) {
		printf("Toggling hitboxes!\n");
		the_game.draw_hitboxes = !the_game.draw_hitboxes;
	}

	switch(the_game.current_mode) {
	case PROGRAM_MENU: {
		handle_menu_inputs();
	} break;

	case PROGRAM_GAME: {
		handle_game_inputs();
	} break;

	}
}

const float player_missile_speed = 700;
const float invader_missile_speed = 300;
const u32 missile_lifetime = 1;

const u32 powerup_duration = 10; // 10 seconds.
const s32 pickup_chance = 33;    // Measured in % chance;
const float chance_to_fire_invader_missile = 30;

const s32 explode_burst_count = 32;
const s32  shield_burst_count = 8;

static void update_app() {
	if (app_should_close)   return;
	if (the_game.is_paused) return;
	if (the_game.is_over)   return;

	if (the_game.current_mode == PROGRAM_GAME) {
		check_level_changes(); // This function is a MESS.

		s32 entity_index = 0;
		For (entities) {
			if (it->is_dead) {
				entity_index += 1;
				continue;
			}

			Entity_Handle handle = {entity_index, it->gen};
			switch (it->type) {
			case ENTITY_PLAYER: {
				it->pos.x += it->vel.x * dt;
				it->pos.y += it->vel.y * dt;
				
				const float player_left   = it->pos.x;
				const float player_right  = it->pos.x + player_width;
				const float player_top    = it->pos.y + player_height;
				const float player_bottom = it->pos.y;
		
				if (player_left < 0) {
					it->pos.x = 0;
				} else if (player_right > window_width) {
					it->pos.x = window_width - player_width;
				}

				if (player_bottom < 0) {
					it->pos.y = 0;
				} else if (player_top > window_height) {
					it->pos.y = window_height - player_height;
				}

				// Tracking the player's powerups.
				if (it->current_pickup != 0)  it->duration -= dt;
				if (it->duration <= 0) {
					it->current_pickup = PICKUP_UNINITIALIZED;
					it->duration = 0;
				}
			} break;

			case ENTITY_INVADER: {
				it->pos.x += it->vel.x * dt;
				it->pos.y += it->vel.y * dt;

				const float invader_bottom = it->pos.y + invader_height;
				const float invader_top = it->pos.y;

				if (invader_top > window_height) {
					destroy_entity(handle);
					the_game.current_invader_count -= 1;
				}

				// Every few seconds, fire_invader_missile() based on RNG.
				it->duration -= dt;
				if (it->duration <= 0) {
					if (GetRandomValue(0, 100) < chance_to_fire_invader_missile) {
						const float center_width = it->pos.x + invader_width*.5f;
						const float bottom = it->pos.y + invader_height;
						fire_invader_missile({center_width - invader_missile_width*.5f, invader_bottom});
					}
					it->duration = GetRandomValue(1000, 3000) / 1000.0f;
				}
			} break;

			case ENTITY_PICKUP: {
				it->pos.y += it->vel.y * dt;

				const float pickup_bottom = it->pos.y + pickup_height;
				const float pickup_top = it->pos.y;
				if (pickup_top > window_height) {
					destroy_entity(handle);
				}
			} break;

			case ENTITY_PLAYER_MISSILE: {
				it->duration += dt;
				it->pos.x += it->vel.x * dt;
				it->pos.y += it->vel.y * dt;

				Particle_Emitter *e = array_get_at_index(&emitters, it->trail_emitter_index);
				e->pos.x = it->pos.x + player_missile_width*.5f;
				e->pos.y = it->pos.y;

				const float missile_bottom = it->pos.y + player_missile_height;
				if (it->duration >= missile_lifetime || missile_bottom < 0) {
					e->is_dead = true;
					destroy_entity(handle);
				}
			} break;
			
			case ENTITY_INVADER_MISSILE: {
				it->pos.x += it->vel.x * dt;
				it->pos.y += it->vel.y * dt;
				
				Particle_Emitter *e = array_get_at_index(&emitters, it->trail_emitter_index);
				e->pos.x = it->pos.x + invader_missile_width*.5f;
				e->pos.y = it->pos.y + invader_missile_height;

				const float missile_top = it->pos.y;
				if (missile_top > window_height) {
					e->is_dead = true;
					destroy_entity(handle);
				}
			} break;
			}
			entity_index += 1;
		}

		//
		// Checking for missile collisions towards invaders.
		//
		for (s32 i = 0; i < (s32)entities.count; ++i) {
			Entity *missile = array_get_at_index(&entities, i);
			if (missile->is_dead || missile->type != ENTITY_PLAYER_MISSILE) continue;
			Rectangle missile_rect = {missile->pos.x, missile->pos.y, player_missile_width, player_missile_height};

			for (s32 j = 0; j < (s32)entities.count; ++j) {
				Entity *enemy = array_get_at_index(&entities, j);
				if (enemy->is_dead || enemy->type != ENTITY_INVADER) continue;
				Rectangle enemy_rect = {enemy->pos.x, enemy->pos.y, invader_width, invader_height};

				if (CheckCollisionRecs(missile_rect, enemy_rect)) {
					if (enemy->current_pickup == PICKUP_SHIELD) {
						enemy->current_pickup = PICKUP_UNINITIALIZED;

						const Vector2 emitter_pos = { enemy->pos.x + invader_width*.5f, enemy->pos.y + invader_height*.5f };
						Particle_Emitter shield_explode = make_exploding_emitter(emitter_pos, BLUE, {0, 0, 255, 0});
						emitter_explode(&shield_explode, shield_burst_count);
						add_emitter(shield_explode);

						// @TODO: Writing this code is easy to forget. Can we make this better?
						Particle_Emitter *trail = array_get_at_index(&emitters, missile->trail_emitter_index);
						trail->is_dead = true;
						destroy_entity({i, missile->gen});
						PlaySound(sound_pickup_power);
						break;
					} 
					if (GetRandomValue(0, 100) < pickup_chance) {
						spawn_random_pickup({enemy->pos.x + (invader_width*.5f), enemy->pos.y + (invader_height*.5f)});
					}

					const Vector2 emitter_pos = { enemy->pos.x + invader_width*.5f, enemy->pos.y + invader_height*.5f };
					Particle_Emitter explode = make_exploding_emitter(emitter_pos, ORANGE, {ORANGE.r, ORANGE.g, ORANGE.b, 0});
					emitter_explode(&explode, explode_burst_count);
					add_emitter(explode);

					Particle_Emitter *trail = array_get_at_index(&emitters, missile->trail_emitter_index);
					trail->is_dead = true;

					destroy_entity({i, missile->gen});
					destroy_entity({j, enemy->gen});
					the_game.current_score += 100;
					the_game.current_invader_count -= 1;
					
					float pitch = (float)(GetRandomValue(70, 90) / 100.0f);
					SetSoundPitch(sound_enemy_dies, pitch);
					PlaySound(sound_enemy_dies);
					break;
				}
			}
		}

		//
		// Checking collisions for the player ship.
		//
		Entity *player = get_entity(player_handle);
		if (player) {
			Rectangle player_rect = {player->pos.x, player->pos.y, player_width, player_height};

			// First check for collisions between the player and the pickups.
			for (s32 p = 0; p < (s32)entities.count; ++p) {
				Entity *pickup = array_get_at_index(&entities, p);
				if (pickup->is_dead || pickup->type != ENTITY_PICKUP) continue;
				Rectangle pickup_rect = {pickup->pos.x, pickup->pos.y, pickup_width, pickup_height};

				if (CheckCollisionRecs(player_rect, pickup_rect)) {
					switch (pickup->pickup_type) {
					case PICKUP_COIN: {
						the_game.current_score += 250;
						PlaySound(sound_pickup_coin);
					} break;

					case PICKUP_SHIELD: {
						player->current_pickup = PICKUP_SHIELD;
						player->duration = powerup_duration;
						PlaySound(sound_pickup_power);
					} break;

					case PICKUP_DOUBLE_SHOT: {
						player->current_pickup = PICKUP_DOUBLE_SHOT;
						player->duration = powerup_duration;
						PlaySound(sound_pickup_power);
					} break;
					}
					destroy_entity({p, pickup->gen});
				}
			}


			// Then check for collisions with invader ships.
			for (s32 j = 0; j < (s32)entities.count; ++j) {
				Entity *enemy = array_get_at_index(&entities, j);
				if (enemy->is_dead || enemy->type != ENTITY_INVADER) continue;
				Rectangle enemy_rect = {enemy->pos.x, enemy->pos.y, invader_width, invader_height};

				if (CheckCollisionRecs(player_rect, enemy_rect)) {
					if (player->current_pickup == PICKUP_SHIELD) {
						const Vector2 emitter_pos = { enemy->pos.x + invader_width*.5f, enemy->pos.y + invader_height*.5f };
						Particle_Emitter explode = make_exploding_emitter(emitter_pos, ORANGE, {});
						emitter_explode(&explode, explode_burst_count);
						add_emitter(explode);

						destroy_entity({j, enemy->gen});
						PlaySound(sound_enemy_dies);
						player->current_pickup = PICKUP_UNINITIALIZED;
						player->duration = 0;
					} else {
						destroy_entity({j, enemy->gen});
						destroy_entity(player_handle);
						PlaySound(sound_player_dies);
						the_game.is_over = true;
					}
					the_game.current_invader_count -= 1;
				}
			}

			// Finally, check for collisions with invader missiles.
			for (s32 m = 0; m < (s32)entities.count; ++m) {
				Entity *missile = array_get_at_index(&entities, m);
				if (missile->is_dead || missile->type != ENTITY_INVADER_MISSILE) continue;
				// We slightly shrink the height of the missile since the hitbox
				// is kinda weird when it's rotated.
				Rectangle missile_rect = {missile->pos.x, missile->pos.y, invader_missile_width, invader_missile_height * .1f};

				if (CheckCollisionRecs(player_rect, missile_rect)) {
					Particle_Emitter *e = array_get_at_index(&emitters, missile->trail_emitter_index);
					e->is_dead = true;
					destroy_entity({m, missile->gen});
					
					if (player->current_pickup == PICKUP_SHIELD) {
						PlaySound(sound_pickup_power); // @Temp.
						player->current_pickup = PICKUP_UNINITIALIZED;
						player->duration = 0;
					} else {
						destroy_entity(player_handle);
						PlaySound(sound_player_dies);
						the_game.is_over = true;
					}
				}				
			}
		}

		// This specifically updates only the particles and not the emitters
		// themselves. Trailing emitter positions are updated within the above
		// procedure.
		update_each_emitters_particles();
	}
}

const float font_size    = 32;
const float ui_font_size = 24;

const float spacing = 0;
const float padding = 100; // This is used to the menu ui.

const Vector2 origin = {};
const float rotation = 0.0f; 

static void draw_space_background() {
	const float scale = 0.75f;
	const Rectangle bg_source = {0.0f, 0.0f, (float)texture_background.width, (float)texture_background.height};
	const Rectangle bg_dest = {0.0f, 0.0f, window_width, window_height};
	draw_texture_tiled(texture_background, bg_source, bg_dest, origin, rotation, scale, WHITE);

	const Color top = {0, 0, 0, 200};
	const Color bottom = {0, 0, 0, 0};
	DrawRectangleGradientV(0, 0, window_width, window_height, top, bottom);
}

static void draw_menu() {
	BeginDrawing();
	ClearBackground(BLACK);

	draw_space_background();

	const float logo_width_half  = texture_logo.width * 0.5f;
	const float logo_height_half = texture_logo.height * 0.5f;

	Rectangle source = {0.0f, 0.0f, (float)texture_logo.width, (float)texture_logo.height};
	Rectangle dest   = {center_width - (logo_width_half*0.5f), window_height * 0.1f, logo_width_half, logo_height_half};
	DrawTexturePro(texture_logo, source, dest, origin, rotation, WHITE);

	Color text_color;

	const char *play_str = "Start game";
	Vector2 text_dim = MeasureTextEx(the_font, play_str, font_size, spacing);
	Vector2 text_pos = {center_width - (text_dim.x*0.5f), center_height};
	if (the_game.current_menu_item == MENU_START_GAME)  text_color = GREEN;
	else												text_color = WHITE; 

	DrawTextEx(the_font, play_str, text_pos, font_size, spacing, text_color);

	const char *quit_str = "Quit";
	text_dim = MeasureTextEx(the_font, quit_str, font_size, spacing);
	text_pos = {center_width - (text_dim.x*0.5f), (text_pos.y + font_size + padding)};
	if (the_game.current_menu_item == MENU_QUIT_GAME)  text_color = GREEN;
	else											   text_color = WHITE; 
	DrawTextEx(the_font, quit_str, text_pos, font_size, spacing, text_color);
	
	EndDrawing();
}

static void draw_paused_menu() {
	Color bg_color = {0, 0, 0, 180};
	Rectangle bg = {0, 0, window_width, window_height};
	DrawRectangleRec(bg, bg_color);

	Color text_color;

	const char *play_str = "Resume";
	Vector2 text_dim = MeasureTextEx(the_font, play_str, font_size, spacing);
	Vector2 text_pos = {center_width - (text_dim.x*0.5f), center_height};
	if (the_game.current_menu_item == MENU_START_GAME)  text_color = GREEN;
	else												text_color = WHITE; 
	DrawTextEx(the_font, play_str, text_pos, font_size, spacing, text_color);

	const char *exit_str = "Back to main menu";
	text_dim = MeasureTextEx(the_font, exit_str, font_size, spacing);
	text_pos = {center_width - (text_dim.x*0.5f), (text_pos.y + font_size + padding)};
	if (the_game.current_menu_item == MENU_QUIT_GAME)  text_color = GREEN;
	else											   text_color = WHITE; 
	DrawTextEx(the_font, exit_str, text_pos, font_size, spacing, text_color);
}

static void draw_game_over_screen() {
	const char *str1 = "Game Over!";
	Vector2 dim = MeasureTextEx(the_font, str1, font_size, spacing);
    Vector2 pos = {center_width - (dim.x*.5f), center_height};
	DrawTextEx(the_font, str1, pos, font_size, spacing, RED);

	const char *str2 = "Press 'R' to restart game.";
	dim = MeasureTextEx(the_font, str2, font_size, spacing);
	pos = {center_width - (dim.x*.5f), pos.y + font_size};
	DrawTextEx(the_font, str2, pos, font_size, spacing, WHITE);
}

const float hitbox_line_thickness = 2.0f;

static void draw_game() {
	BeginDrawing();
	ClearBackground(BLACK);

	draw_space_background();

	For (entities) {
		if (it->is_dead) continue;

		switch (it->type) {
		case ENTITY_PLAYER: {
			Rectangle source = {0.0f, 0.0f, (float)it->map->width, (float)it->map->height};
			Rectangle dest = {it->pos.x, it->pos.y, player_width, player_height};
			DrawTexturePro(*it->map, source, dest, origin, rotation, WHITE);

			const Vector2 player_center = {it->pos.x + (player_width*.5f), it->pos.y+ (player_height*.5f)};
			const float circle_radius = player_width * 0.7f;
			Color circle_color;
			u8 opacity; // Ranges from 0-255.

			if (it->duration > 3) opacity = 100;
			else                  opacity = 50;

			if (it->duration != 0) {
				switch (it->current_pickup) {
				case PICKUP_SHIELD: {
					circle_color = {0, 0, 255, opacity};
					DrawCircleV(player_center, circle_radius, circle_color);
				} break;

				case PICKUP_DOUBLE_SHOT: {
					circle_color = {255, 0, 0, opacity};
					DrawCircleV(player_center, circle_radius, circle_color);
				} break;
				}
			}

			if (the_game.draw_hitboxes) {
				DrawRectangleLinesEx(dest, hitbox_line_thickness, PINK);
			}
		} break;

		case ENTITY_INVADER: {
			Rectangle source = {0.0f, 0.0f, (float)it->map->width, (float)it->map->height};
			Rectangle dest = {it->pos.x, it->pos.y, invader_width, invader_height};
			DrawTexturePro(*it->map, source, dest, origin, rotation, WHITE);

			if (it->current_pickup == PICKUP_SHIELD) {
				const Vector2 invader_center = {it->pos.x + (invader_width*.5f), it->pos.y + (invader_height*.5f)};
				const float circle_radius = invader_width * 0.55f;
				const Color circle_color  = {0, 0, 255, 100};
				DrawCircleV(invader_center, circle_radius, circle_color);
			}

			if (the_game.draw_hitboxes) {
				DrawRectangleLinesEx(dest, hitbox_line_thickness, PINK);
			}

		} break;

		case ENTITY_PICKUP: {
			Rectangle source = {0.0f, 0.0f, (float)it->map->width, (float)it->map->height};
			Rectangle dest = {it->pos.x, it->pos.y, pickup_width, pickup_height};
			DrawTexturePro(*it->map, source, dest, origin, rotation, WHITE);
			if (the_game.draw_hitboxes) {
				DrawRectangleLinesEx(dest, hitbox_line_thickness, PINK);
			}

		} break;

		case ENTITY_PLAYER_MISSILE: {
			Rectangle source = {0.0f, 0.0f, (float)it->map->width, (float)it->map->height};
			Rectangle dest = {it->pos.x, it->pos.y, player_missile_width, player_missile_height};
			DrawTexturePro(*it->map, source, dest, origin, rotation, WHITE);
			if (the_game.draw_hitboxes) {
				DrawRectangleLinesEx(dest, hitbox_line_thickness, PINK);
			}
		} break;

		case ENTITY_INVADER_MISSILE: {
			// @TODO: So we want to rotate the texture to be upside down since the
			// asset we have makes it so the bullet is facing upwards, therefore
			// for the invaders' missiles, we want the visual of the missile going
			// downwards. This rotation paramater does do that, but the pivot point
			// of the missile is not at the texture's center, which displays an
			// inaccurate drawing of where the hitbox actually is. Therefore we
			// need to figure this out eventually.
//			const float rotation = 180;
			Rectangle source = {0.0f, 0.0f, (float)it->map->width, (float)it->map->height};
			Rectangle dest = {it->pos.x, it->pos.y, invader_missile_width, invader_missile_height};
			DrawTexturePro(*it->map, source, dest, origin, rotation, RED);
			if (the_game.draw_hitboxes) {
				DrawRectangleLinesEx(dest, hitbox_line_thickness, PINK);
			}
		} break;
		}
	}

	draw_emitters();

	//
	// UI stuff.
	//
	const Color text_color = GREEN;
	Vector2 text_pos = {0, 0};
	char score[64];
	char level[64];
	char invaders[64];
	snprintf(score, sizeof(score), "Score: %d", the_game.current_score);
	snprintf(level, sizeof(level), "Level: %d", the_game.current_level);
	snprintf(invaders, sizeof(invaders), "Invaders remaining: %d", the_game.current_invader_count);

	DrawTextEx(the_font, level, text_pos, ui_font_size, spacing, text_color);
	text_pos.y += ui_font_size;
	DrawTextEx(the_font, score, text_pos, ui_font_size, spacing, text_color);
	text_pos.y += ui_font_size;
	DrawTextEx(the_font, invaders, text_pos, ui_font_size, spacing, text_color);

	Entity *player = get_entity(player_handle);
	if (player->current_pickup != 0) {
		char powerup[64];
		char duration[64];
		const char *power;
		if (player->current_pickup == PICKUP_SHIELD) {
			power = "SHIELD";
		} else if (player->current_pickup == PICKUP_DOUBLE_SHOT) {
			power = "DOUBLE SHOT";
		}
		snprintf(powerup, sizeof(powerup), "Current Power: %s", power);
		snprintf(duration, sizeof(duration), "Duration: %.1f", player->duration);
	
		text_pos = {0, window_height - ui_font_size};
		DrawTextEx(the_font, duration, text_pos, ui_font_size, spacing, text_color);
		text_pos.y -= ui_font_size;
		DrawTextEx(the_font, powerup, text_pos, ui_font_size, spacing, text_color);
	}
	
	if (the_game.is_over)   draw_game_over_screen();
	if (the_game.is_paused)	draw_paused_menu();

	if (time_until_game_close <= 2.9f) { // Kinda hacky way since I got lazy.
		const char *success = "Congrats! That's the end of the game.";
		const Vector2 dim = MeasureTextEx(the_font, success, font_size, spacing);
		const Vector2 pos = {center_width - (dim.x*.5f), window_height * 0.9f};
		DrawTextEx(the_font, success, pos, font_size, spacing, GREEN);
	}
		

	EndDrawing();
}

static void draw_app() {
	if (the_game.current_mode == PROGRAM_MENU) {
		draw_menu();
	} else if (the_game.current_mode == PROGRAM_GAME) {
		draw_game();
	}
}

static Entity_Handle add_entity(Entity e = {}) {
	e.gen = 0;

	// Loop through the entities array and check each entity for an is_dead flag.
	// If we find one, replace the new entity and overwrite the dead one.
	s32 i = 0;
	For (entities) {
		if (it->is_dead) {
			s32 new_gen = it->gen + 1;
			*it = e;
			it->is_dead = false;
			it->gen = new_gen;
			return {i, new_gen};
		}
		++i;
	}

	s32 index = (s32)entities.count;
	array_add(&entities, e);
	Entity *entity = array_get_at_index(&entities, index);
	entity->gen = 1;
	return {index, entity->gen};
}

//
// We never remove entities from the Auto_Array<T>, but we instead just modify
// the is_dead flag to true. This is because we replace those entity slots with
// the add_entity() procedure.
//
// To add, we technically don't need to call this procedure if we are already
// within a loop that iterates through pointers of the entities array. We can just
// do:        it->is_dead = true;
//
static void destroy_entity(Entity_Handle handle) {
	Entity *e = get_entity(handle);
	if (!e)  return;
	e->is_dead = true;
}

static Entity *get_entity(Entity_Handle handle) {
	if (handle.id < 0 || handle.id >= (s32)entities.count)  return nullptr;

	Entity *e = array_get_at_index(&entities, handle.id);
	if (e->gen != handle.gen)  return nullptr;
	return e;
}

static void fire_player_missile() {
	float pitch;
	float horizontal = 0;
	Entity *player = get_entity(player_handle);
	if (player->vel.x < 0) {
		horizontal = -100;
	} else if (player->vel.x > 0){
		horizontal =  100;
	}

	if (player->current_pickup == PICKUP_DOUBLE_SHOT) {
		const u32 missile_margin = 5;

		Entity missile1 = {};
		missile1.pos = {player->pos.x + missile_margin, player->pos.y};
		missile1.vel = {horizontal, -player_missile_speed};
		missile1.map = &texture_missile;
		missile1.type = ENTITY_PLAYER_MISSILE;
		missile1.is_dead = false;

		Entity missile2 = missile1;
		missile2.pos.x = player->pos.x + player_width - missile_margin;
		
		Particle_Emitter trail1 = make_trailing_emitter_for_missile(missile1.pos, ENTITY_PLAYER_MISSILE);
		Particle_Emitter trail2 = make_trailing_emitter_for_missile(missile2.pos, ENTITY_PLAYER_MISSILE);
		missile1.trail_emitter_index = add_emitter(trail1);
		missile2.trail_emitter_index = add_emitter(trail2);

		add_entity(missile1);
		add_entity(missile2);
		
		for (s32 i = 0; i < 2; ++i) {
			pitch = (float)(GetRandomValue(60, 130) / 100.0f);
			SetSoundPitch(sound_player_shoot, pitch);
			PlaySound(sound_player_shoot);
		}

	} else {
		Entity missile = {};
		missile.pos = {player->pos.x + (player_width*.5f), player->pos.y};
		missile.vel = {horizontal, -player_missile_speed};
		missile.map = &texture_missile;
		missile.type = ENTITY_PLAYER_MISSILE;
		missile.is_dead = false;
	
		Particle_Emitter trail = make_trailing_emitter_for_missile(missile.pos, ENTITY_PLAYER_MISSILE);
		missile.trail_emitter_index = add_emitter(trail);

		add_entity(missile);

		pitch = (float)(GetRandomValue(60, 130) / 100.0f);
		SetSoundPitch(sound_player_shoot, pitch);
		PlaySound(sound_player_shoot);
	}
}

static void fire_invader_missile(Vector2 pos) {
	Entity missile = {};
	missile.pos = pos;
	missile.vel = {0, invader_missile_speed};
	missile.map = &texture_missile;
	missile.type = ENTITY_INVADER_MISSILE;
	missile.is_dead = false;

	Particle_Emitter trail = make_trailing_emitter_for_missile(missile.pos, ENTITY_INVADER_MISSILE);
	missile.trail_emitter_index = add_emitter(trail);

	add_entity(missile);
}

const s32 invader_spawn_rate = 5;

static void spawn_invaders() {
	const float invader_y_max = 0;
	const float invader_y_min = invader_y_max - 400;
	
	const s32 invaders_to_spawn = the_game.current_level * invader_spawn_rate;
	the_game.current_invader_count = invaders_to_spawn;

	const float left_threshold = 0; 
	const float right_threshold = window_width - invader_width;

	s32 chance_to_spawn_with_shield = 0;
	if (the_game.current_level > 2) {
		chance_to_spawn_with_shield = 25;
	}

	for (s32 i = 0; i < invaders_to_spawn; ++i) {
		Entity invader = {};
		invader.pos.x = GetRandomValue(left_threshold, right_threshold);
		invader.pos.y = GetRandomValue(invader_y_max, invader_y_min);
		invader.vel = {0, (float)GetRandomValue(invader_movement_speed_min, invader_movement_speed_max)};
		invader.map = &texture_enemy_ship;
		invader.type = ENTITY_INVADER;
		invader.is_dead = false;

		if (GetRandomValue(0, 100) < chance_to_spawn_with_shield) {
			invader.current_pickup = PICKUP_SHIELD;
		} else {
			invader.current_pickup = PICKUP_UNINITIALIZED;
		}
		
		// This is used for the invader's fire cooldowns.
		invader.duration = GetRandomValue(1000, 3000) / 1000.0f; // 1-3 seconds.

		add_entity(invader);
	}
}

static void spawn_random_pickup(Vector2 pos) {
	Entity e = {};
	e.pos = pos;
	e.vel = {0, pickup_movement_speed};
	e.is_dead = false;

	e.type = ENTITY_PICKUP;
	e.pickup_type = static_cast<Pickup_Type>(GetRandomValue(PICKUP_COIN, PICKUP_DOUBLE_SHOT));
	
	switch (e.pickup_type) {
	case PICKUP_COIN: {
		e.map = &texture_coin;
	} break;

	case PICKUP_SHIELD: {
		e.map = &texture_missing; // @Temp.
	} break;

	case PICKUP_DOUBLE_SHOT: {
		e.map = &texture_missing; // @Temp.
	} break;
	}

	add_entity(e);
}

// This is has a bugs in it that I can't be asked to fix just yet... therefore
// @TODO: Make this better!!!
static void check_level_changes() {
	if (the_game.current_level > level_cap) {
		time_until_game_close -= dt;
		if (time_until_game_close <= 0) {
			app_should_close = true;
		}
	}		
		
	if (the_game.current_invader_count == 0) {
		if (the_game.current_level < level_cap) {
			the_game.current_level += 1;
			spawn_invaders();
		}
	}
}

// Copied this code from a raylib example since there is no DrawTextureTiled()
// procedure that is within the raylib header file, therefore this code is just
// dumped here to get the tiled background rendering.
void draw_texture_tiled(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, float scale, Color tint) {
    if ((texture.id <= 0)   || (scale <= 0.0f))      return;
    if ((source.width == 0) || (source.height == 0)) return;

    s32 tile_width = (int)(source.width*scale), tile_height = (int)(source.height*scale);
    if ((dest.width < tile_width) && (dest.height < tile_height)) {
		DrawTexturePro(texture,
					   {source.x, source.y, ((float)dest.width/tile_width)*source.width, ((float)dest.height/tile_height)*source.height},
					   {dest.x, dest.y, dest.width, dest.height},
					   origin, rotation, tint);
		
    } else if (dest.width <= tile_width) {  // Tiled vertically.
        s32 dy = 0;
        for (;dy + tile_height < dest.height; dy += tile_height) {
            DrawTexturePro(texture,
						   {source.x, source.y, ((float)dest.width/tile_width)*source.width, source.height},
						   {dest.x, dest.y + dy, dest.width, (float)tile_height},
						   origin, rotation, tint);
        }

        if (dy < dest.height) {  // Fit last tile.
            DrawTexturePro(texture,
						   {source.x, source.y, ((float)dest.width/tile_width)*source.width, ((float)(dest.height - dy)/tile_height)*source.height},
						   {dest.x, dest.y + dy, dest.width, dest.height - dy},
						   origin, rotation, tint);
        }
		
    } else if (dest.height <= tile_height) {  // Tiled horizontally.
        s32 dx = 0;
        for (;dx + tile_width < dest.width; dx += tile_width) {
            DrawTexturePro(texture,
						   {source.x, source.y, source.width, ((float)dest.height/tile_height)*source.height},
						   {dest.x + dx, dest.y, (float)tile_width, dest.height},
						   origin, rotation, tint);
        }

        if (dx < dest.width) {  // Fit last tile.
            DrawTexturePro(texture,
						   {source.x, source.y, ((float)(dest.width - dx)/tile_width)*source.width, ((float)dest.height/tile_height)*source.height},
						   {dest.x + dx, dest.y, dest.width - dx, dest.height},
						   origin, rotation, tint);
        }
		
    } else {  // Tiled both horizontally and vertically (rows and columns).
        s32 dx = 0;
        for (;dx + tile_width < dest.width; dx += tile_width) {
            s32 dy = 0;
            for (;dy + tile_height < dest.height; dy += tile_height) {
                DrawTexturePro(texture,
							   source,
							   {dest.x + dx, dest.y + dy, (float)tile_width, (float)tile_height},
							   origin, rotation, tint);
            }

            if (dy < dest.height) {
                DrawTexturePro(texture,
							   {source.x, source.y, source.width, ((float)(dest.height - dy)/tile_height)*source.height},
							   {dest.x + dx, dest.y + dy, (float)tile_width, dest.height - dy},
							   origin, rotation, tint);
            }
        }

        if (dx < dest.width) {  // Fit last column of tiles.
			s32 dy = 0;
            for (;dy + tile_height < dest.height; dy += tile_height) {
                DrawTexturePro(texture,
							   {source.x, source.y, ((float)(dest.width - dx)/tile_width)*source.width, source.height},
							   {dest.x + dx, dest.y + dy, dest.width - dx, (float)tile_height},
							   origin, rotation, tint);
            }
   
			if (dy < dest.height) {  // Draw final tile in the bottom right corner.
                DrawTexturePro(texture,
							   {source.x, source.y, ((float)(dest.width - dx)/tile_width)*source.width, ((float)(dest.height - dy)/tile_height)*source.height},
							   {dest.x + dx, dest.y + dy, dest.width - dx, dest.height - dy},
							   origin, rotation, tint);
            }
        }
    }
}

const float explode_particle_radius_min = 4.0f;
const float explode_particle_radius_max = 6.0f;

static Particle_Emitter make_exploding_emitter(Vector2 pos, Color color_start, Color color_end = {0, 0, 0, 0}) {
	Particle_Emitter emitter = {};
	emitter.pos = pos;
	emitter.dir = {};

	emitter.min_lifetime = 0.5f;
	emitter.max_lifetime = 1.0f;

	emitter.min_size = explode_particle_radius_min;
	emitter.max_size = explode_particle_radius_max;

	emitter.color_start = color_start;
	emitter.color_end   = color_end;

	emitter.type = EMITTER_EXPLODE;
	emitter.is_dead = false;

	return emitter;
}

const float trail_particle_radius_min = 2.0f;
const float trail_particle_radius_max = 3.0f;

static Particle_Emitter make_trailing_emitter_for_missile(Vector2 pos, Entity_Type type = ENTITY_UNINITIALIZED) {
	Particle_Emitter emitter = {};
	if (type == ENTITY_UNINITIALIZED) return emitter;

	if (type == ENTITY_PLAYER_MISSILE) {
		emitter.type = EMITTER_PLAYER_MISSILE_TRAIL;
		emitter.color_start = {102, 244, 255, 255};
	} else if (type == ENTITY_INVADER_MISSILE) {
		emitter.type = EMITTER_INVADER_MISSILE_TRAIL;
		emitter.color_start = RED;
	} 
	emitter.color_end = {25, 25, 25, 0};
	
	emitter.pos = pos;
	emitter.dir = {0, 1};

	emitter.min_lifetime = 0.1f;
	emitter.max_lifetime = 0.3f;

	emitter.min_size = trail_particle_radius_min;
	emitter.max_size = trail_particle_radius_max;

	emitter.is_dead = false;

	return emitter;
}

static void emitter_explode(Particle_Emitter *emitter, s32 burst_count) {
	for (s32 i = 0; i < burst_count; ++i) spawn_explode_particle(emitter);
}

static s32 add_emitter(Particle_Emitter emitter = {}) {
	s32 index = 0;
	For (emitters) {
		if (it->is_dead) { // Replace any dead emitters.
			*it = emitter;
			it->is_dead = false;
			return index;
		}
		index += 1;
	}

	array_add(&emitters, emitter);
	return (s32)emitters.count - 1;
}

static void spawn_explode_particle(Particle_Emitter *emitter) {
	assert(emitter);
	
	Particle *p = NULL;
	for (s32 i = 0; i < MAX_PARTICLES_PER_EMITTER; ++i) {
		if (!emitter->particles[i].is_active) {
			p = &emitter->particles[i];
			break;
		}
	}
	if (!p) { printf("Particles pool for this emitter is full!\n"); return; }

	// Initialize particle from its emitter.
	float angle = GetRandomValue(0, 360) * DEG2RAD;
	float speed = 50.0f + GetRandomValue(0, 100);

	p->pos = emitter->pos;
	p->vel = { cosf(angle) * speed, sinf(angle) * speed };
	
	p->color = emitter->color_start;

	p->elapsed = 0;
	p->lifetime = emitter->min_lifetime + GetRandomValue(0, 100) / 100.0f * (emitter->max_lifetime - emitter->min_lifetime);

	if (emitter->min_size == 0) {
		p->size = emitter->max_size;
	} else {
		p->size = (float)(GetRandomValue(emitter->min_size, emitter->max_size)); 
	}
	p->is_active = true;
}

const float  player_trail_vel_y = 60.0f;
const float invader_trail_vel_y = -80.0f;

static void spawn_trail_particle(Particle_Emitter *emitter) {
	assert(emitter);

	Particle *p = NULL;
	for (s32 i = 0; i < MAX_PARTICLES_PER_EMITTER; ++i) {
		if (!emitter->particles[i].is_active) {
			p = &emitter->particles[i];
			break;
		}
	}
	if (!p) { printf("Particles pool for this emitter is full!\n"); return; }

	float sideways = GetRandomValue(-30, 30);
	p->pos = emitter->pos;
	p->vel = {sideways, 0};

	if (emitter->type == EMITTER_PLAYER_MISSILE_TRAIL) {
		p->vel.y = player_trail_vel_y;
	} else if (emitter->type == EMITTER_INVADER_MISSILE_TRAIL) {
		p->vel.y = invader_trail_vel_y;
	}
	p->color = emitter->color_start;
	p->elapsed = 0.0f;
	p->lifetime = emitter->min_lifetime + GetRandomValue(0, 100) / 100.0f * (emitter->max_lifetime - emitter->min_lifetime);

	p->size = (float)GetRandomValue(emitter->min_size, emitter->max_size);

	p->is_active = true;
}

static void update_each_emitters_particles() {
	For (emitters) {
		if (it->is_dead) continue;

		switch (it->type) {
		case EMITTER_EXPLODE: {
			bool any_alive = false;
			for (s32 i = 0; i < MAX_PARTICLES_PER_EMITTER; ++i) {
				Particle *p = &it->particles[i];
				if (!p->is_active) continue;
				p->elapsed += dt;

				float t = p->elapsed / p->lifetime;
				p->color = ColorLerp(it->color_start, it->color_end, t); 
				p->pos.x += p->vel.x * dt;
				p->pos.y += p->vel.y * dt;
			
				if (p->elapsed >= p->lifetime) {
					p->is_active = false;
				} else {
					any_alive = true;
				}
			}
			if (!any_alive) it->is_dead = true;
		} break;

			
		case EMITTER_INVADER_MISSILE_TRAIL:
		case EMITTER_PLAYER_MISSILE_TRAIL: 
			spawn_trail_particle(it);
			for (s32 i = 0; i < MAX_PARTICLES_PER_EMITTER; ++i) {
				Particle *p = &it->particles[i];
				if (!p->is_active) continue;
				p->elapsed += dt;

				float t = p->elapsed / p->lifetime;
				p->color = ColorLerp(it->color_start, it->color_end, t); 
				p->pos.x += p->vel.x * dt;
				p->pos.y += p->vel.y * dt;

				if (p->elapsed >= p->lifetime) p->is_active = false;
				// The emitter will die when the missile is dead.
			}
			break;
		}
	}
}


static void draw_emitters() {
	For (emitters) {
		if (it->is_dead) continue;

		switch (it->type) {
		case EMITTER_EXPLODE:
		case EMITTER_PLAYER_MISSILE_TRAIL:
		case EMITTER_INVADER_MISSILE_TRAIL:
			for (s32 i = 0; i < MAX_PARTICLES_PER_EMITTER; ++i) {
				Particle *p = &it->particles[i];
				if (!p->is_active) continue;
				DrawCircleV(p->pos, p->size, p->color);
			}
			break;
		}

	}
}
