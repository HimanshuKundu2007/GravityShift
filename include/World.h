#pragma once
#include "Player.h"
#include "Camera.h"
#include "Angel/Angel.h"
#include "Room.h"
#include "LevelManager.h"
#include "FMOD/fmod.hpp"
#include <vector>

/**
 * @brief Top-level game world orchestrator for Gravity Shift.
 * 
 * Coordinates the 3D space station room geometry, player astronaut controller,
 * multi-level puzzle objectives, camera pipeline, audio playback via FMOD,
 * and boundary collision detection.
 */
class World {
public:
	enum State {
		RUN = 0,      ///< Active gameplay execution
		PAUSE = 1,    ///< Simulation paused
		ONE_FRAME = 2 ///< Single-frame step simulation
	};

	World(GLuint program);
	~World();

	// Simulation & Rendering
	void draw();
	void updateWorld(float dt = 0.016f);
	void restartRoom();
	void checkCollision();
	void setProjection(float aspect);

	// Ceiling Visibility Toggle
	void toggleCeiling() { if (room) room->toggleCeiling(); }
	bool isCeilingVisible() const { return room ? room->isCeilingVisible() : true; }

	// Gameplay Actions
	void cycleGravity();
	void interact();

	// Core Subsystems
	Camera* camera;
	Player* player;
	Room* room;
	LevelManager* levelManager;

	// State & Transformations
	State worldState;
	GLuint mainVAO;
	GLuint frameCounter;
	GLuint program_ptr;
	GLuint ModelView_ptr;
	GLuint Projection_ptr;
	mat4 projection;

private:
	void initSounds();

	// FMOD Audio Subsystem
	FMOD::System *FMODSystem;
	FMOD::Channel *ambienceChannel;
	FMOD::Channel *engineChannel;
	FMOD::Channel *effectChannel;
	FMOD::Sound *ambience;
	FMOD::Sound *engine_loop;
	FMOD::Sound *engine_startup;
	FMOD::Sound *blaster;
	FMOD::Sound *explosion;
};