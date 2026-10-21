#include "World.h"
#include <cmath>
#include <iostream>

#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

World::World(GLuint program) {
	program_ptr = program;
	frameCounter = 0;
	worldState = State::RUN;

	// 1. Initialize Room environment and Level Manager
	room = new Room(program_ptr);
	levelManager = new LevelManager(program_ptr);

	// 2. Instantiate Player at Level 1 spawn coordinates
	vec4 spawnPos = levelManager ? levelManager->getPlayerSpawn() : vec4(0.0f, -6.5f, 0.40f, 1.0f);
	player = new Player(spawnPos, 0, 0, 0, -1);

	// 3. Attach tracking camera to player
	camera = new Camera();
	camera->attach(player);

	// 4. Set up OpenGL Vertex Array Object (VAO) and Buffer for Player Astronaut mesh
	GLuint pointsBytes = sizeof(vec4) * player->points.size();
	GLuint normalsBytes = sizeof(vec3) * player->normals.size();
	GLuint totalBytes = pointsBytes + normalsBytes;

	glGenVertexArrays(1, &mainVAO);
	glBindVertexArray(mainVAO);

	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, totalBytes, NULL, GL_STATIC_DRAW);

	// Upload vertex positions [0 .. pointsBytes]
	glBufferSubData(GL_ARRAY_BUFFER, 0, pointsBytes, player->points.front());
	// Upload vertex normals [pointsBytes .. pointsBytes + normalsBytes]
	glBufferSubData(GL_ARRAY_BUFFER, pointsBytes, normalsBytes, player->normals.front());

	// Bind vPosition vertex attribute (location 0, vec4)
	GLuint vPosition = glGetAttribLocation(program_ptr, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	// Bind vNormal vertex attribute (location 1, vec3)
	GLuint vNormal = glGetAttribLocation(program_ptr, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(pointsBytes));

	// 5. Setup Lighting and Shader Uniforms
	vec4 light_position(0.0, 0.0, 0.0, 1.0);
	glUniform4fv(glGetUniformLocation(program_ptr, "LightPosition"), 1, light_position);

	ModelView_ptr = glGetUniformLocation(program_ptr, "ModelView");
	Projection_ptr = glGetUniformLocation(program_ptr, "Projection");

	setProjection(960.0f / 780.0f);

	// 6. Initialize Audio
	initSounds();

	if (player && levelManager) {
		player->spawnPosition = levelManager->getPlayerSpawn();
		player->resetToSpawn();
	}

	engineChannel = nullptr;
	effectChannel = nullptr;
	ambienceChannel = nullptr;
	if (FMODSystem && ambience) {
		FMODSystem->playSound(ambience, 0, false, &ambienceChannel);
		if (ambienceChannel) ambienceChannel->setVolume(0.20f);
	}
}

World::~World() {
	if (camera) delete camera;
	if (room) delete room;
	if (levelManager) delete levelManager;
	if (player) delete player;

	if (FMODSystem) {
		FMODSystem->close();
		FMODSystem->release();
	}
}

void World::initSounds() {
	FMOD::System_Create(&FMODSystem);
	FMODSystem->init(32, FMOD_INIT_NORMAL, 0);
	FMODSystem->createStream("media/ambience.wav", FMOD_DEFAULT, 0, &ambience);
	if (ambience) ambience->setMode(FMOD_LOOP_NORMAL);
	FMODSystem->createSound("media/engine_loop.wav", FMOD_DEFAULT, 0, &engine_loop);
	if (engine_loop) engine_loop->setMode(FMOD_LOOP_NORMAL);
	FMODSystem->createSound("media/engine_startup.wav", FMOD_DEFAULT, 0, &engine_startup);
	FMODSystem->createSound("media/blaster.wav", FMOD_DEFAULT, 0, &blaster);
	FMODSystem->createSound("media/explosion.wav", FMOD_DEFAULT, 0, &explosion);
}

void World::cycleGravity() {
	if (!player) return;
	player->cycleGravity();
	if (FMODSystem && engine_startup) {
		FMODSystem->playSound(engine_startup, 0, false, &effectChannel);
		if (effectChannel) effectChannel->setVolume(0.55f);
	}
}

void World::interact() {
	if (!player || !levelManager) return;

	// Calculate 3D facing vector from surface orientation and yaw
	vec3 facing = player->getFacingVector();

	LevelManager::InteractResult res = levelManager->tryInteract(player->position, facing);
	if (res == LevelManager::INTERACT_KEYCARD) {
		if (FMODSystem && blaster) {
			FMODSystem->playSound(blaster, 0, false, &effectChannel);
			if (effectChannel) effectChannel->setVolume(0.70f);
		}
	} else if (res == LevelManager::INTERACT_CRATE) {
		if (FMODSystem && explosion) {
			FMODSystem->playSound(explosion, 0, false, &effectChannel);
			if (effectChannel) effectChannel->setVolume(0.35f);
		}
	} else if (res == LevelManager::INTERACT_EXIT) {
		player->velocity = vec3(0.0f, 0.0f, 0.0f);
		if (FMODSystem && engine_startup) {
			FMODSystem->playSound(engine_startup, 0, false, &effectChannel);
			if (effectChannel) effectChannel->setVolume(0.85f);
		}
	} else if (res == LevelManager::INTERACT_ACCESS_DENIED) {
		if (FMODSystem && explosion) {
			FMODSystem->playSound(explosion, 0, false, &effectChannel);
			if (effectChannel) effectChannel->setVolume(0.40f);
		}
	}
}

/**
 * @brief Master simulation update step called every frame.
 * 
 * Sequentially updates:
 * 1. FMOD Audio engine
 * 2. Room animations and particle effects
 * 3. Modal game state and level completion animations
 * 4. Player kinematics and gravity integration
 * 5. Level objective tracking (crates, keycard, portal)
 * 6. AABB boundary collisions and grounding resolution
 * 7. Camera tracking
 */
void World::updateWorld(float dt) {
	if (FMODSystem) {
		FMODSystem->update();
	}
	frameCounter++;

	if (room) {
		room->update(dt);
	}

	// Pause world simulation and player motion during modal completion screens
	if (levelManager && levelManager->getState() != LevelManager::GameState::PLAYING) {
		levelManager->updateLevelCompleteAnim(dt);
		if (player) {
			player->velocity = vec3(0.0f, 0.0f, 0.0f);
			player->zeroGVelocity = vec3(0.0f, 0.0f, 0.0f);
			player->currentSpeed = 0.0f;
		}
		camera->update();
		return;
	}

	// Update player physics
	if (player) {
		player->updatePhysics(dt);
	}

	// Update LevelManager (tracks objectives, crates, obstacles, keycard)
	if (levelManager && player) {
		int prevLvl = levelManager->getCurrentLevel();
		float hX, hY, hZ;
		player->getEffectiveHalfExtents(hX, hY, hZ);
		levelManager->update(dt, player->position, player->velocity, hX, hY, hZ);
		if (levelManager->getCurrentLevel() != prevLvl) {
			player->spawnPosition = levelManager->getPlayerSpawn();
			player->resetToSpawn();
		}
	}

	// Check boundary collisions across all 4 room surfaces
	checkCollision();

	// Update camera following astronaut
	camera->update();
}

void World::restartRoom() {
	if (room) {
		room->resetEffects();
	}
	if (levelManager) {
		levelManager->restartLevel();
	}
	if (player && levelManager) {
		player->spawnPosition = levelManager->getPlayerSpawn();
		player->resetToSpawn();
	}
}

/**
 * @brief Collision detection and penetration resolution algorithm.
 * 
 * Performs Axis-Aligned Bounding Box (AABB) vs. Half-Space Plane intersection tests
 * against the room's 6 boundary planes:
 * 1. Floor: Z = Room::MIN_Z (0.0)
 * 2. Ceiling: Z = Room::MAX_Z (8.0)
 * 3. Left Wall: X = Room::MIN_X (-10.0)
 * 4. Right Wall: X = Room::MAX_X (10.0)
 * 5. Rear Wall: Y = Room::MIN_Y (-22.0)
 * 6. Front Wall: Y = Room::MAX_Y (24.0)
 * 
 * Penetration is resolved by clamping player position along the contact normal
 * and canceling normal velocity. Sets isGrounded = true when contacting the surface
 * opposing the active gravity vector.
 */
void World::checkCollision() {
	if (!player || !room) return;

	// Use gravity-aware effective half extents based on surface orientation
	float rX, rY, rZ;
	player->getEffectiveHalfExtents(rX, rY, rZ);

	bool touchedGroundedSurface = false;

	// 1. FLOOR (Z = Room::MIN_Z = 0.0)
	if (player->position.z - rZ < Room::MIN_Z) {
		player->position.z = Room::MIN_Z + rZ;
		if (player->velocity.z < 0.0f) {
			player->velocity.z = 0.0f;
		}
		if (player->currentGravity == Player::GRAV_FLOOR) {
			touchedGroundedSurface = true;
		}
	}

	// 2. CEILING (Z = Room::MAX_Z = 8.0)
	if (player->position.z + rZ > Room::MAX_Z) {
		player->position.z = Room::MAX_Z - rZ;
		if (player->velocity.z > 0.0f) {
			player->velocity.z = 0.0f;
		}
		if (player->currentGravity == Player::GRAV_CEILING) {
			touchedGroundedSurface = true;
		}
	}

	// 3. LEFT WALL (X = Room::MIN_X = -10.0)
	if (player->position.x - rX < Room::MIN_X) {
		player->position.x = Room::MIN_X + rX;
		if (player->velocity.x < 0.0f) {
			player->velocity.x = 0.0f;
		}
		if (player->currentGravity == Player::GRAV_LEFT_WALL) {
			touchedGroundedSurface = true;
		}
	}

	// 4. RIGHT WALL (X = Room::MAX_X = 10.0)
	if (player->position.x + rX > Room::MAX_X) {
		player->position.x = Room::MAX_X - rX;
		if (player->velocity.x > 0.0f) {
			player->velocity.x = 0.0f;
		}
		if (player->currentGravity == Player::GRAV_RIGHT_WALL) {
			touchedGroundedSurface = true;
		}
	}

	// 5. REAR WALL (Y = Room::MIN_Y = -22.0)
	if (player->position.y - rY < Room::MIN_Y) {
		player->position.y = Room::MIN_Y + rY;
		if (player->velocity.y < 0.0f) {
			player->velocity.y = 0.0f;
		}
	}

	// 6. FRONT WALL / EXIT PORTAL (Y = Room::MAX_Y = 24.0)
	if (player->position.y + rY > Room::MAX_Y) {
		player->position.y = Room::MAX_Y - rY;
		if (player->velocity.y > 0.0f) {
			player->velocity.y = 0.0f;
		}
	}

	// Also check if standing on top of obstacle or crate
	if (!touchedGroundedSurface && levelManager && player->currentGravity == Player::GRAV_FLOOR) {
		if (levelManager->isPlayerSupported(player->position, rX, rY, rZ)) {
			touchedGroundedSurface = true;
			if (player->velocity.z < 0.0f) {
				player->velocity.z = 0.0f;
			}
		}
	}

	player->isGrounded = touchedGroundedSurface;
}

/**
 * @brief Computes perspective projection matrix using vertical FOV (60 deg), aspect ratio,
 * near clip plane (0.2m), and far clip plane (150m).
 */
void World::setProjection(float aspect) {
	projection = Perspective(60.0f, aspect, 0.20f, 150.0f);
	if (program_ptr != 0) {
		glUseProgram(program_ptr);
		glUniformMatrix4fv(Projection_ptr, 1, GL_TRUE, projection);
	}
}

/**
 * @brief Master 3D scene rendering pass.
 * 
 * Pipeline:
 * 1. Transforms point light source into camera view space
 * 2. Renders room architecture (floors, walls, pillars, ceiling, lights)
 * 3. Renders level objectives (portal, crates, keycard, forcefield)
 * 4. Renders player astronaut with multi-material Phong shading
 */
void World::draw() {
	// 1. Position overhead research station light fixture dynamically above active player sector
	float lightY = player ? (player->position.y + 1.5f) : 2.0f;
	if (lightY < Room::MIN_Y + 3.0f) lightY = Room::MIN_Y + 3.0f;
	if (lightY > Room::MAX_Y - 3.0f) lightY = Room::MAX_Y - 3.0f;
	glUniform4fv(glGetUniformLocation(program_ptr, "LightPosition"), 1, camera->view * vec4(0.0f, lightY, 7.2f, 1.0f));

	// 2. Draw Space Station Architecture
	if (room) {
		room->draw(camera->view, program_ptr, ModelView_ptr);
	}

	// 3. Draw Level-specific Elements (Obstacles, Crates, Keycard, Forcefield)
	if (levelManager) {
		levelManager->draw(camera->view, program_ptr, ModelView_ptr);
	}

	// 4. Draw Player Astronaut
	glBindVertexArray(mainVAO);
	if (player) {
		player->draw(camera->view, program_ptr, ModelView_ptr);
	}
	glBindVertexArray(0);
}