#include "LevelManager.h"
#include "Room.h"
#include <iostream>
#include <fstream>
#include <cmath>

LevelManager::LevelManager(GLuint program)
	: currentLevel(1)
	, currentState(GameState::PLAYING)
	, levelElapsedTime(0.0f)
	, totalGameTime(0.0f)
	, transitionCooldown(0.0f)
	, currentCollisions(0)
	, collisionCooldown(0.0f)
	, currentRestarted(false)
	, totalScore(0)
	, hasKeycardObjective(false)
	, hasMovableCrate(false)
	, oxygenActive(false)
	, oxygenRemaining(0.0f)
	, exitTriggerY(20.2f)
	, vao(0)
	, vbo(0)
	, bufferDirty(true)
	, obsOffset(0), obsCount(0)
	, crateOffset(0), crateCount(0)
	, keyOffset(0), keyCount(0)
	, barrierOffset(0), barrierCount(0)
	, pulseTimer(0.0f)
	, crateMovedFeedbackTimer(0.0f)
	, objectiveFeedbackTimer(0.0f)
	, levelCompleteAnimTimer(0.0f)
	, levelIntroTimer(3.5f)
{
	keycard.active = false;
	keycard.collected = false;
	keycard.spinAngle = 0.0f;

	crate.active = false;
	crate.pos = vec3(0.0f, 0.0f, 0.0f);
	crate.halfSize = vec3(1.1f, 1.0f, 1.1f);
	crate.vel = vec3(0.0f, 0.0f, 0.0f);

	for (int i = 0; i < 5; i++) {
		levelRecords[i] = LevelScoreRecord();
	}

	initMaterials();
	loadLevel(1);
	rebuildGeometry(program);
}

LevelManager::~LevelManager() {
	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

void LevelManager::initMaterials() {
	vec4 light_ambient(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_diffuse(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_specular(1.0f, 1.0f, 1.0f, 1.0f);

	// 1. Obstacle barriers (Electrified Amber/Graphite Hazard fields)
	obstacleMat.material_ambient = vec4(0.28f, 0.16f, 0.06f, 1.0f);
	obstacleMat.material_diffuse = vec4(0.80f, 0.44f, 0.12f, 1.0f);
	obstacleMat.material_specular = vec4(0.70f, 0.55f, 0.35f, 1.0f);
	obstacleMat.material_shininess = 50.0f;
	obs_ambient = light_ambient * obstacleMat.material_ambient;
	obs_diffuse = light_diffuse * obstacleMat.material_diffuse;
	obs_specular = light_specular * obstacleMat.material_specular;

	// 2. Movable heavy crate (Deep industrial alloy with hazard edge brackets)
	crateMat.material_ambient = vec4(0.14f, 0.18f, 0.22f, 1.0f);
	crateMat.material_diffuse = vec4(0.38f, 0.44f, 0.52f, 1.0f);
	crateMat.material_specular = vec4(0.60f, 0.70f, 0.80f, 1.0f);
	crateMat.material_shininess = 60.0f;
	crate_ambient = light_ambient * crateMat.material_ambient;
	crate_diffuse = light_diffuse * crateMat.material_diffuse;
	crate_specular = light_specular * crateMat.material_specular;

	// 3. Holographic Keycard (Bright Gold-Cyan emissive crystal)
	keycardMat.material_ambient = vec4(0.85f, 0.72f, 0.18f, 1.0f);
	keycardMat.material_diffuse = vec4(0.95f, 0.85f, 0.28f, 1.0f);
	keycardMat.material_specular = vec4(1.00f, 1.00f, 0.90f, 1.0f);
	keycardMat.material_shininess = 100.0f;
	key_ambient = light_ambient * keycardMat.material_ambient;
	key_diffuse = light_diffuse * keycardMat.material_diffuse;
	key_specular = light_specular * keycardMat.material_specular;

	// 4. Locked Blast Forcefield Barrier (Vivid Red-Orange Alert)
	barrierLockedMat.material_ambient = vec4(0.75f, 0.10f, 0.08f, 1.0f);
	barrierLockedMat.material_diffuse = vec4(0.90f, 0.18f, 0.14f, 1.0f);
	barrierLockedMat.material_specular = vec4(0.90f, 0.45f, 0.45f, 1.0f);
	barrierLockedMat.material_shininess = 80.0f;
	lock_ambient = light_ambient * barrierLockedMat.material_ambient;
	lock_diffuse = light_diffuse * barrierLockedMat.material_diffuse;
	lock_specular = light_specular * barrierLockedMat.material_specular;

	// 5. Unlocked Blast Forcefield Barrier (Clean Cyan-Emerald Authorization)
	barrierUnlockedMat.material_ambient = vec4(0.10f, 0.75f, 0.40f, 1.0f);
	barrierUnlockedMat.material_diffuse = vec4(0.18f, 0.90f, 0.55f, 1.0f);
	barrierUnlockedMat.material_specular = vec4(0.60f, 1.00f, 0.80f, 1.0f);
	barrierUnlockedMat.material_shininess = 90.0f;
	unlock_ambient = light_ambient * barrierUnlockedMat.material_ambient;
	unlock_diffuse = light_diffuse * barrierUnlockedMat.material_diffuse;
	unlock_specular = light_specular * barrierUnlockedMat.material_specular;
}

void LevelManager::loadLevel(int lvl) {
	currentLevel = lvl;
	currentState = GameState::PLAYING;
	levelElapsedTime = 0.0f;
	currentCollisions = 0;
	collisionCooldown = 0.0f;
	currentRestarted = false;
	obstacles.clear();
	bufferDirty = true;

	hasKeycardObjective = false;
	keycard.active = false;
	keycard.collected = false;

	hasMovableCrate = false;
	crate.active = false;
	crate.vel = vec3(0.0f, 0.0f, 0.0f);

	oxygenActive = false;
	oxygenRemaining = 0.0f;

	pulseTimer = 0.0f;
	crateMovedFeedbackTimer = 0.0f;
	objectiveFeedbackTimer = 0.0f;
	levelCompleteAnimTimer = 0.0f;
	levelIntroTimer = 3.5f;

	switch (currentLevel) {
	case 1:
		// -------------------------------------------------------------
		// LEVEL 1: BASIC MOVEMENT
		// Floor walking, jumping, reaching the exit door and pressing [E].
		// -------------------------------------------------------------
		levelName = "LEVEL 1: BASIC MOVEMENT";
		objectiveText = "WALK FORWARD [W/S, A/D, SPACE]. APPROACH EXIT DOOR AND PRESS [E] TO EXIT.";
		playerSpawnPos = vec4(0.0f, -16.0f, 0.90f, 1.0f);
		break;

	case 2:
		// -------------------------------------------------------------
		// LEVEL 2: WALL WALKING
		// Massive hazard field blocks floor path. Shift gravity [G] to Left Wall.
		// -------------------------------------------------------------
		levelName = "LEVEL 2: WALL WALKING";
		objectiveText = "FLOOR IS BLOCKED! SHIFT GRAVITY [G] TO LEFT WALL TO TRAVERSE TO EXIT.";
		playerSpawnPos = vec4(0.0f, -18.0f, 0.90f, 1.0f);

		// Electrified containment barrier spanning center floor
		// Leaves Left Wall (X <= -7.8) and Right Wall (X >= 7.8) completely open for wall-walking!
		obstacles.push_back({ -7.8f, 7.8f, -8.0f, 14.0f, 0.0f, 4.2f, 0 });
		break;

	case 3:
		// -------------------------------------------------------------
		// LEVEL 3: CRATE PUZZLE
		// Heavy cargo crate blocks blast door. Push crate aside with [E] or walking.
		// -------------------------------------------------------------
		levelName = "LEVEL 3: CRATE PUZZLE";
		objectiveText = "CARGO CRATE BLOCKS THE BLAST DOOR! PUSH CRATE ASIDE TO CLEAR THE EXIT.";
		playerSpawnPos = vec4(0.0f, -16.0f, 0.90f, 1.0f);

		hasMovableCrate = true;
		crate.active = true;
		crate.pos = vec3(0.0f, 18.2f, 1.10f);
		crate.halfSize = vec3(1.20f, 1.00f, 1.10f);
		crate.vel = vec3(0.0f, 0.0f, 0.0f);
		break;

	case 4:
		// -------------------------------------------------------------
		// LEVEL 4: FIND THE KEYCARD
		// Door locked! Keycard is on Ceiling. Shift gravity [G] to Ceiling to collect.
		// -------------------------------------------------------------
		levelName = "LEVEL 4: FIND THE KEYCARD";
		objectiveText = "DOOR LOCKED! SHIFT GRAVITY [G] TO CEILING TO COLLECT KEYCARD [E], THEN UNLOCK DOOR.";
		playerSpawnPos = vec4(0.0f, -16.0f, 0.90f, 1.0f);

		hasKeycardObjective = true;
		keycard.active = true;
		keycard.collected = false;
		keycard.pos = vec3(0.0f, 0.0f, 7.30f); // Mounted high on Ceiling
		keycard.spinAngle = 0.0f;
		break;

	case 5:
		// -------------------------------------------------------------
		// LEVEL 5: GRAVITY MASTER
		// Floor barrier, Keycard on Right Wall, Crate puzzle, Pod Exit, Oxygen timer.
		// -------------------------------------------------------------
		levelName = "LEVEL 5: GRAVITY MASTER";
		objectiveText = "EVACUATION EMERGENCY! RETRIEVE KEYCARD, CLEAR CRATE, AND REACH ESCAPE POD!";
		playerSpawnPos = vec4(0.0f, -18.0f, 0.90f, 1.0f);

		oxygenActive = true;
		oxygenRemaining = 65.0f;

		// Floor hazard field in forward sector
		obstacles.push_back({ -7.8f, 7.8f, -10.0f, 2.0f, 0.0f, 4.2f, 0 });

		// Keycard mounted on Right Wall at Z = 4.0m
		hasKeycardObjective = true;
		keycard.active = true;
		keycard.collected = false;
		keycard.pos = vec3(9.25f, 6.0f, 4.00f);
		keycard.spinAngle = 0.0f;

		// Movable crate in front of door approach (blocks aperture until pushed aside)
		hasMovableCrate = true;
		crate.active = true;
		crate.pos = vec3(0.0f, 17.8f, 1.10f);
		crate.halfSize = vec3(1.20f, 1.00f, 1.10f);
		crate.vel = vec3(0.0f, 0.0f, 0.0f);
		break;
	}
}

void LevelManager::restartLevel() {
	loadLevel(currentLevel);
	currentRestarted = true;
}

void LevelManager::replayCurrentLevel() {
	loadLevel(currentLevel);
	currentRestarted = false;
	currentCollisions = 0;
}

void LevelManager::proceedToNextLevel() {
	if (currentLevel < 5) {
		loadLevel(currentLevel + 1);
	} else {
		currentState = GameState::GAME_COMPLETE;
	}
}

void LevelManager::nextLevel() {
	proceedToNextLevel();
}

void LevelManager::replayGame() {
	for (int i = 0; i < 5; i++) {
		levelRecords[i] = LevelScoreRecord();
	}
	totalScore = 0;
	loadLevel(1);
}

void LevelManager::recordCollision() {
	if (currentState != GameState::PLAYING) return;
	if (collisionCooldown <= 0.0f) {
		currentCollisions++;
		collisionCooldown = 0.40f;
	}
}

bool LevelManager::isExitBlocked() const {
	if (hasKeycardObjective && !keycard.collected) {
		return true;
	}
	if (hasMovableCrate && crate.active) {
		// Crate blocks doorway if in aperture: |X| < 1.9 and Y > 17.0
		if (std::abs(crate.pos.x) < 1.9f && crate.pos.y > 17.0f) {
			return true;
		}
	}
	return false;
}

int LevelManager::getLiveLevelScore() const {
	int base = 1000;
	int sec = (int)std::floor(levelElapsedTime);
	int penalty = sec * 10;
	int bonus = 0;
	if (currentCollisions == 0) bonus += 100;
	if (!currentRestarted) bonus += 100;
	int s = base - penalty + bonus;
	return (s < 0) ? 0 : s;
}

/**
 * @brief Level completion and multi-factor scoring calculation.
 * 
 * Evaluates performance metrics upon reaching the unlocked exit portal:
 * - Base completion points: 1000 pts
 * - Elapsed time deduction: -10 pts per full second elapsed
 * - Zero collision bonus: +100 pts if currentCollisions == 0
 * - Clean attempt bonus: +100 pts if level was completed without restarting
 * - Score lower-bound clamping: Score >= 0
 * 
 * Stores best score record, accumulates total mission score, and transitions
 * state to LEVEL_COMPLETE (or GAME_COMPLETE for Level 5).
 */
void LevelManager::completeCurrentLevel() {
	if (currentLevel < 1 || currentLevel > 5) return;

	LevelScoreRecord rec;
	rec.completed = true;
	rec.timeTaken = levelElapsedTime;
	rec.basePoints = 1000;
	rec.timePenalty = (int)std::floor(levelElapsedTime) * 10;
	rec.noCollisionBonus = (currentCollisions == 0) ? 100 : 0;
	rec.noRestartBonus = (!currentRestarted) ? 100 : 0;
	rec.collisions = currentCollisions;
	rec.restarted = currentRestarted;

	int calc = rec.basePoints - rec.timePenalty + rec.noCollisionBonus + rec.noRestartBonus;
	rec.finalScore = (calc < 0) ? 0 : calc;

	// Keep best score for this level upon replay
	if (!levelRecords[currentLevel - 1].completed || rec.finalScore > levelRecords[currentLevel - 1].finalScore) {
		levelRecords[currentLevel - 1] = rec;
	}
	currentResult = rec;

	// Calculate overall total score
	totalScore = 0;
	for (int i = 0; i < 5; i++) {
		if (levelRecords[i].completed) {
			totalScore += levelRecords[i].finalScore;
		}
	}

	if (currentLevel == 5) {
		currentState = GameState::GAME_COMPLETE;
	} else {
		currentState = GameState::LEVEL_COMPLETE;
	}
	levelCompleteAnimTimer = 0.0f;
}

float LevelManager::getTotalCompletedTime() const {
	float t = 0.0f;
	for (int i = 0; i < 5; i++) {
		if (levelRecords[i].completed) {
			t += levelRecords[i].timeTaken;
		}
	}
	return t;
}

std::string LevelManager::getInteractionPrompt(const vec4& playerPos) const {
	if (currentState != GameState::PLAYING) return "";

	// 1. Keycard Proximity Prompt
	if (keycard.active && !keycard.collected) {
		float dx = playerPos.x - keycard.pos.x;
		float dy = playerPos.y - keycard.pos.y;
		float dz = playerPos.z - keycard.pos.z;
		if (std::sqrt(dx * dx + dy * dy + dz * dz) < 2.5f) {
			return "[E] COLLECT KEYCARD";
		}
	}

	// 2. Movable Crate Proximity Prompt
	if (hasMovableCrate && crate.active) {
		float dx = playerPos.x - crate.pos.x;
		float dy = playerPos.y - crate.pos.y;
		float dz = playerPos.z - crate.pos.z;
		if (std::sqrt(dx * dx + dy * dy + dz * dz) < 2.8f) {
			return "[E] PUSH CARGO CRATE";
		}
	}

	// 3. Exit Door Proximity Prompt
	if (playerPos.y >= 19.8f && std::abs(playerPos.x) <= 2.8f) {
		if (hasKeycardObjective && !keycard.collected) {
			return "[E] ACCESS DOOR (LOCKED - KEYCARD REQUIRED)";
		}
		if (hasMovableCrate && crate.active && std::abs(crate.pos.x) < 1.9f && crate.pos.y > 17.0f) {
			return "[E] DOOR BLOCKED BY CARGO CRATE";
		}
		return "[E] EXIT SECTOR";
	}

	return "";
}

LevelManager::InteractResult LevelManager::tryInteract(const vec4& playerPos, const vec3& facingDir) {
	if (currentState != GameState::PLAYING) return INTERACT_NONE;

	// 1. Keycard Interaction
	if (keycard.active && !keycard.collected) {
		float dx = playerPos.x - keycard.pos.x;
		float dy = playerPos.y - keycard.pos.y;
		float dz = playerPos.z - keycard.pos.z;
		if (std::sqrt(dx * dx + dy * dy + dz * dz) < 2.5f) {
			keycard.collected = true;
			bufferDirty = true;
			triggerObjectiveFeedback();
			return INTERACT_KEYCARD;
		}
	}

	// 2. Movable Crate Interaction
	if (hasMovableCrate && crate.active) {
		float dx = playerPos.x - crate.pos.x;
		float dy = playerPos.y - crate.pos.y;
		float dz = playerPos.z - crate.pos.z;
		if (std::sqrt(dx * dx + dy * dy + dz * dz) < 2.8f) {
			vec3 pushDir = facingDir;
			if (length(pushDir) < 0.1f) pushDir = vec3(0.0f, 1.0f, 0.0f);
			crate.vel += normalize(pushDir) * 4.5f;
			triggerCrateMovedFeedback();
			return INTERACT_CRATE;
		}
	}

	// 3. Exit Door Interaction
	if (playerPos.y >= 19.8f && std::abs(playerPos.x) <= 2.8f) {
		if (hasKeycardObjective && !keycard.collected) {
			recordCollision();
			return INTERACT_ACCESS_DENIED;
		}
		if (hasMovableCrate && crate.active && std::abs(crate.pos.x) < 1.9f && crate.pos.y > 17.0f) {
			recordCollision();
			return INTERACT_CRATE;
		}

		// Exit satisfied!
		completeCurrentLevel();
		return INTERACT_EXIT;
	}

	return INTERACT_NONE;
}

void LevelManager::update(float dt, vec4& playerPos, vec3& playerVel, float hX, float hY, float hZ) {
	if (currentState != GameState::PLAYING) return;

	levelElapsedTime += dt;
	totalGameTime += dt;
	pulseTimer += dt;

	if (crateMovedFeedbackTimer > 0.0f) {
		crateMovedFeedbackTimer -= dt;
		if (crateMovedFeedbackTimer < 0.0f) crateMovedFeedbackTimer = 0.0f;
	}
	if (objectiveFeedbackTimer > 0.0f) {
		objectiveFeedbackTimer -= dt;
		if (objectiveFeedbackTimer < 0.0f) objectiveFeedbackTimer = 0.0f;
	}
	if (levelIntroTimer > 0.0f) {
		levelIntroTimer -= dt;
		if (levelIntroTimer < 0.0f) levelIntroTimer = 0.0f;
	}

	if (transitionCooldown > 0.0f) transitionCooldown -= dt;
	if (collisionCooldown > 0.0f)  collisionCooldown -= dt;

	// 1. Oxygen countdown (Level 5)
	if (oxygenActive) {
		oxygenRemaining -= dt;
		if (oxygenRemaining <= 0.0f) {
			oxygenRemaining = 0.0f;
			currentState = GameState::OXYGEN_DEPLETED;
			return;
		}
	}

	// 2. Keycard rotation
	if (keycard.active && !keycard.collected) {
		keycard.spinAngle += dt * 140.0f;
		if (keycard.spinAngle >= 360.0f) keycard.spinAngle -= 360.0f;
	}

	// 3. Movable crate physics: Semi-implicit Euler integration & Coulomb friction damping
	if (hasMovableCrate && crate.active) {
		crate.pos += crate.vel * dt;
		crate.vel *= (1.0f - dt * 2.8f); // Exponential velocity decay to simulate surface friction

		// Constrain crate within room bulkheads (elastic reflection damping on walls)
		if (crate.pos.x - crate.halfSize.x < Room::MIN_X + 0.3f) {
			crate.pos.x = Room::MIN_X + 0.3f + crate.halfSize.x;
			crate.vel.x = -crate.vel.x * 0.2f;
		}
		if (crate.pos.x + crate.halfSize.x > Room::MAX_X - 0.3f) {
			crate.pos.x = Room::MAX_X - 0.3f - crate.halfSize.x;
			crate.vel.x = -crate.vel.x * 0.2f;
		}
		if (crate.pos.y - crate.halfSize.y < Room::MIN_Y + 0.3f) {
			crate.pos.y = Room::MIN_Y + 0.3f + crate.halfSize.y;
			crate.vel.y = -crate.vel.y * 0.2f;
		}
		if (crate.pos.y + crate.halfSize.y > Room::MAX_Y - 0.3f) {
			crate.pos.y = Room::MAX_Y - 0.3f - crate.halfSize.y;
			crate.vel.y = -crate.vel.y * 0.2f;
		}
		if (crate.pos.z - crate.halfSize.z < Room::MIN_Z + 0.05f) {
			crate.pos.z = Room::MIN_Z + 0.05f + crate.halfSize.z;
			crate.vel.z = 0.0f;
		}
		if (crate.pos.z + crate.halfSize.z > Room::MAX_Z - 0.05f) {
			crate.pos.z = Room::MAX_Z - 0.05f - crate.halfSize.z;
			crate.vel.z = 0.0f;
		}

		// Constrain crate against stationary obstacles
		for (const auto& obs : obstacles) {
			float cMinX = crate.pos.x - crate.halfSize.x, cMaxX = crate.pos.x + crate.halfSize.x;
			float cMinY = crate.pos.y - crate.halfSize.y, cMaxY = crate.pos.y + crate.halfSize.y;
			float cMinZ = crate.pos.z - crate.halfSize.z, cMaxZ = crate.pos.z + crate.halfSize.z;

			if (cMaxX > obs.minX && cMinX < obs.maxX &&
			    cMaxY > obs.minY && cMinY < obs.maxY &&
			    cMaxZ > obs.minZ && cMinZ < obs.maxZ)
			{
				float ox1 = cMaxX - obs.minX, ox2 = obs.maxX - cMinX;
				float ox = (ox1 < ox2) ? ox1 : ox2;
				float oy1 = cMaxY - obs.minY, oy2 = obs.maxY - cMinY;
				float oy = (oy1 < oy2) ? oy1 : oy2;
				float oz1 = cMaxZ - obs.minZ, oz2 = obs.maxZ - cMinZ;
				float oz = (oz1 < oz2) ? oz1 : oz2;

				if (ox <= oy && ox <= oz) {
					if (crate.pos.x < (obs.minX + obs.maxX) * 0.5f) {
						crate.pos.x = obs.minX - crate.halfSize.x;
						if (crate.vel.x > 0.0f) crate.vel.x = 0.0f;
					} else {
						crate.pos.x = obs.maxX + crate.halfSize.x;
						if (crate.vel.x < 0.0f) crate.vel.x = 0.0f;
					}
				} else if (oy <= ox && oy <= oz) {
					if (crate.pos.y < (obs.minY + obs.maxY) * 0.5f) {
						crate.pos.y = obs.minY - crate.halfSize.y;
						if (crate.vel.y > 0.0f) crate.vel.y = 0.0f;
					} else {
						crate.pos.y = obs.maxY + crate.halfSize.y;
						if (crate.vel.y < 0.0f) crate.vel.y = 0.0f;
					}
				} else {
					if (crate.pos.z < (obs.minZ + obs.maxZ) * 0.5f) {
						crate.pos.z = obs.minZ - crate.halfSize.z;
						if (crate.vel.z > 0.0f) crate.vel.z = 0.0f;
					} else {
						crate.pos.z = obs.maxZ + crate.halfSize.z;
						if (crate.vel.z < 0.0f) crate.vel.z = 0.0f;
					}
				}
			}
		}

		// Player-to-Crate AABB collision detection & dynamic impulse transfer:
		// Resolves overlap along minimum penetration axis and imparts velocity impulse to crate
		float pMinX = playerPos.x - hX, pMaxX = playerPos.x + hX;
		float pMinY = playerPos.y - hY, pMaxY = playerPos.y + hY;
		float pMinZ = playerPos.z - hZ, pMaxZ = playerPos.z + hZ;

		float cMinX = crate.pos.x - crate.halfSize.x, cMaxX = crate.pos.x + crate.halfSize.x;
		float cMinY = crate.pos.y - crate.halfSize.y, cMaxY = crate.pos.y + crate.halfSize.y;
		float cMinZ = crate.pos.z - crate.halfSize.z, cMaxZ = crate.pos.z + crate.halfSize.z;

		if (pMaxX > cMinX && pMinX < cMaxX &&
		    pMaxY > cMinY && pMinY < cMaxY &&
		    pMaxZ > cMinZ && pMinZ < cMaxZ)
		{
			float ox1 = pMaxX - cMinX, ox2 = cMaxX - pMinX;
			float ox = (ox1 < ox2) ? ox1 : ox2;
			float oy1 = pMaxY - cMinY, oy2 = cMaxY - pMinY;
			float oy = (oy1 < oy2) ? oy1 : oy2;
			float oz1 = pMaxZ - cMinZ, oz2 = cMaxZ - pMinZ;
			float oz = (oz1 < oz2) ? oz1 : oz2;

			triggerCrateMovedFeedback();
			if (oy <= ox && oy <= oz) {
				if (playerPos.y < crate.pos.y) {
					playerPos.y = cMinY - hY;
					crate.vel.y += 2.5f;
					if (playerVel.y > 0.0f) playerVel.y = 0.0f;
				} else {
					playerPos.y = cMaxY + hY;
					crate.vel.y -= 2.5f;
					if (playerVel.y < 0.0f) playerVel.y = 0.0f;
				}
			} else if (ox <= oy && ox <= oz) {
				if (playerPos.x < crate.pos.x) {
					playerPos.x = cMinX - hX;
					crate.vel.x += 2.5f;
					if (playerVel.x > 0.0f) playerVel.x = 0.0f;
				} else {
					playerPos.x = cMaxX + hX;
					crate.vel.x -= 2.5f;
					if (playerVel.x < 0.0f) playerVel.x = 0.0f;
				}
			} else {
				if (playerPos.z < crate.pos.z) {
					playerPos.z = cMinZ - hZ;
					if (playerVel.z > 0.0f) playerVel.z = 0.0f;
				} else {
					playerPos.z = cMaxZ + hZ;
					if (playerVel.z < 0.0f) playerVel.z = 0.0f;
				}
			}
		}
	}

	// 4. Player vs Stationary Obstacles
	for (const auto& obs : obstacles) {
		float pMinX = playerPos.x - hX, pMaxX = playerPos.x + hX;
		float pMinY = playerPos.y - hY, pMaxY = playerPos.y + hY;
		float pMinZ = playerPos.z - hZ, pMaxZ = playerPos.z + hZ;

		if (pMaxX > obs.minX && pMinX < obs.maxX &&
		    pMaxY > obs.minY && pMinY < obs.maxY &&
		    pMaxZ > obs.minZ && pMinZ < obs.maxZ)
		{
			recordCollision();

			float ox1 = pMaxX - obs.minX, ox2 = obs.maxX - pMinX;
			float ox = (ox1 < ox2) ? ox1 : ox2;
			float oy1 = pMaxY - obs.minY, oy2 = obs.maxY - pMinY;
			float oy = (oy1 < oy2) ? oy1 : oy2;
			float oz1 = pMaxZ - obs.minZ, oz2 = obs.maxZ - pMinZ;
			float oz = (oz1 < oz2) ? oz1 : oz2;

			if (ox <= oy && ox <= oz) {
				if (playerPos.x < (obs.minX + obs.maxX) * 0.5f) {
					playerPos.x = obs.minX - hX;
					if (playerVel.x > 0.0f) playerVel.x = 0.0f;
				} else {
					playerPos.x = obs.maxX + hX;
					if (playerVel.x < 0.0f) playerVel.x = 0.0f;
				}
			} else if (oy <= ox && oy <= oz) {
				if (playerPos.y < (obs.minY + obs.maxY) * 0.5f) {
					playerPos.y = obs.minY - hY;
					if (playerVel.y > 0.0f) playerVel.y = 0.0f;
				} else {
					playerPos.y = obs.maxY + hY;
					if (playerVel.y < 0.0f) playerVel.y = 0.0f;
				}
			} else {
				if (playerPos.z < (obs.minZ + obs.maxZ) * 0.5f) {
					playerPos.z = obs.minZ - hZ;
					if (playerVel.z > 0.0f) playerVel.z = 0.0f;
				} else {
					playerPos.z = obs.maxZ + hZ;
					if (playerVel.z < 0.0f) playerVel.z = 0.0f;
				}
			}
		}
	}

	// 5. Door Lock Forcefield Barrier Collision (When Locked)
	if (hasKeycardObjective && !keycard.collected) {
		const float barMinX = -2.6f, barMaxX = 2.6f;
		const float barMinY = 20.6f, barMaxY = 21.4f;
		const float barMinZ = 0.0f,  barMaxZ = 4.6f;

		if (playerPos.x + hX > barMinX && playerPos.x - hX < barMaxX &&
		    playerPos.y + hY > barMinY && playerPos.y - hY < barMaxY &&
		    playerPos.z + hZ > barMinZ && playerPos.z - hZ < barMaxZ)
		{
			recordCollision();
			playerPos.y = barMinY - hY;
			if (playerVel.y > 0.0f) playerVel.y = 0.0f;
		}
	}
}

bool LevelManager::isPlayerSupported(const vec4& playerPos, float hX, float hY, float hZ) const {
	float pMinX = playerPos.x - hX, pMaxX = playerPos.x + hX;
	float pMinY = playerPos.y - hY, pMaxY = playerPos.y + hY;
	float pFootZ = playerPos.z - hZ;

	if (hasMovableCrate && crate.active) {
		float cMinX = crate.pos.x - crate.halfSize.x, cMaxX = crate.pos.x + crate.halfSize.x;
		float cMinY = crate.pos.y - crate.halfSize.y, cMaxY = crate.pos.y + crate.halfSize.y;
		float cTopZ = crate.pos.z + crate.halfSize.z;
		if (pMaxX > cMinX && pMinX < cMaxX && pMaxY > cMinY && pMinY < cMaxY) {
			if (std::abs(pFootZ - cTopZ) < 0.12f) return true;
		}
	}

	for (const auto& obs : obstacles) {
		if (pMaxX > obs.minX && pMinX < obs.maxX && pMaxY > obs.minY && pMinY < obs.maxY) {
			if (std::abs(pFootZ - obs.maxZ) < 0.12f) return true;
		}
	}

	return false;
}

void LevelManager::addQuad(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3,
                           std::vector<vec4>& pts, std::vector<vec3>& norms) {
	vec4 u = v1 - v0;
	vec4 v = v2 - v0;
	vec3 n = normalize(cross(u, v));

	pts.push_back(v0); norms.push_back(n);
	pts.push_back(v1); norms.push_back(n);
	pts.push_back(v2); norms.push_back(n);
	pts.push_back(v0); norms.push_back(n);
	pts.push_back(v2); norms.push_back(n);
	pts.push_back(v3); norms.push_back(n);
}

void LevelManager::addBox(float minX, float minY, float minZ,
                          float maxX, float maxY, float maxZ,
                          std::vector<vec4>& pts, std::vector<vec3>& norms) {
	vec4 v000(minX, minY, minZ, 1.0f);
	vec4 v100(maxX, minY, minZ, 1.0f);
	vec4 v110(maxX, maxY, minZ, 1.0f);
	vec4 v010(minX, maxY, minZ, 1.0f);
	vec4 v001(minX, minY, maxZ, 1.0f);
	vec4 v101(maxX, minY, maxZ, 1.0f);
	vec4 v111(maxX, maxY, maxZ, 1.0f);
	vec4 v011(minX, maxY, maxZ, 1.0f);

	addQuad(v000, v100, v110, v010, pts, norms);
	addQuad(v001, v011, v111, v101, pts, norms);
	addQuad(v000, v001, v101, v100, pts, norms);
	addQuad(v110, v111, v011, v010, pts, norms);
	addQuad(v000, v010, v011, v001, pts, norms);
	addQuad(v100, v101, v111, v110, pts, norms);
}

void LevelManager::rebuildGeometry(GLuint program) {
	std::vector<vec4> obsPts, cratePts, keyPts, barrierPts;
	std::vector<vec3> obsNrms, crateNrms, keyNrms, barrierNrms;

	// 1. Obstacle geometry
	for (const auto& obs : obstacles) {
		addBox(obs.minX, obs.minY, obs.minZ, obs.maxX, obs.maxY, obs.maxZ, obsPts, obsNrms);
	}

	// 2. Movable crate unit geometry (drawn at origin, translated in draw)
	addBox(-crate.halfSize.x, -crate.halfSize.y, -crate.halfSize.z,
	        crate.halfSize.x,  crate.halfSize.y,  crate.halfSize.z, cratePts, crateNrms);

	// 3. Keycard item geometry (drawn at origin, translated/spun in draw)
	addBox(-0.30f, -0.04f, -0.45f, 0.30f, 0.04f, 0.45f, keyPts, keyNrms);

	// 4. Forcefield barrier geometry across door aperture
	addBox(-2.4f, 21.0f, 0.05f, 2.4f, 21.2f, 4.6f, barrierPts, barrierNrms);

	obsOffset = 0;
	obsCount = (GLsizei)obsPts.size();

	crateOffset = obsOffset + obsCount;
	crateCount = (GLsizei)cratePts.size();

	keyOffset = crateOffset + crateCount;
	keyCount = (GLsizei)keyPts.size();

	barrierOffset = keyOffset + keyCount;
	barrierCount = (GLsizei)barrierPts.size();

	GLsizei totalVerts = barrierOffset + barrierCount;

	std::vector<vec4> allPts;
	std::vector<vec3> allNrms;
	allPts.reserve(totalVerts);
	allNrms.reserve(totalVerts);

	auto append = [&](const std::vector<vec4>& p, const std::vector<vec3>& n) {
		allPts.insert(allPts.end(), p.begin(), p.end());
		allNrms.insert(allNrms.end(), n.begin(), n.end());
	};

	append(obsPts, obsNrms);
	append(cratePts, crateNrms);
	append(keyPts, keyNrms);
	append(barrierPts, barrierNrms);

	if (vao == 0) glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	if (vbo == 0) glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLsizeiptr posBytes = totalVerts * sizeof(vec4);
	GLsizeiptr normBytes = totalVerts * sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, posBytes + normBytes, NULL, GL_STATIC_DRAW);
	if (totalVerts > 0) {
		glBufferSubData(GL_ARRAY_BUFFER, 0, posBytes, allPts.data());
		glBufferSubData(GL_ARRAY_BUFFER, posBytes, normBytes, allNrms.data());
	}

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)posBytes);

	glBindVertexArray(0);
	bufferDirty = false;
}

void LevelManager::draw(mat4 view, GLuint program, GLuint ModelView) {
	if (bufferDirty) {
		rebuildGeometry(program);
	}
	if (vao == 0) return;

	glBindVertexArray(vao);

	auto setMat = [&](const vec4& amb, const vec4& diff, const vec4& spec, float shin) {
		glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, amb);
		glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diff);
		glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, spec);
		glUniform1f(glGetUniformLocation(program, "Shininess"), shin);
	};

	// 1. Draw Obstacles
	if (obsCount > 0) {
		setMat(obs_ambient, obs_diffuse, obs_specular, obstacleMat.material_shininess);
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, view);
		glDrawArrays(GL_TRIANGLES, obsOffset, obsCount);
	}

	// 2. Draw Movable Crate (with subtle cyan idle accent and responsive push feedback)
	if (hasMovableCrate && crate.active && crateCount > 0) {
		float activeNorm = (crateMovedFeedbackTimer > 0.0f) ? (crateMovedFeedbackTimer / 0.50f) : 0.0f;
		float idleNorm = 0.5f + 0.5f * std::sin(pulseTimer * 3.0f);
		vec4 crateCyan(0.04f + 0.18f * activeNorm,
		               0.10f + 0.55f * activeNorm + 0.08f * idleNorm,
		               0.16f + 0.70f * activeNorm + 0.12f * idleNorm,
		               0.0f);
		setMat(crate_ambient + crateCyan * 0.4f,
		       crate_diffuse + crateCyan * 0.8f,
		       crate_specular + crateCyan,
		       crateMat.material_shininess + activeNorm * 35.0f);
		mat4 crateModel = view * Translate(crate.pos);
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, crateModel);
		glDrawArrays(GL_TRIANGLES, crateOffset, crateCount);
	}

	// 3. Draw Keycard Item (Subtle cyan-gold holographic breathing pulse)
	if (keycard.active && !keycard.collected && keyCount > 0) {
		float keyNorm = 0.5f + 0.5f * std::sin(pulseTimer * 3.8f);
		vec4 keyCyan(0.08f * keyNorm, 0.35f * keyNorm, 0.50f * keyNorm, 0.0f);
		setMat(key_ambient + keyCyan * 0.5f,
		       key_diffuse + keyCyan * 0.8f,
		       key_specular + keyCyan,
		       keycardMat.material_shininess + keyNorm * 25.0f);
		mat4 keyModel = view * Translate(keycard.pos) * RotateZ(keycard.spinAngle) * RotateY(25.0f);
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, keyModel);
		glDrawArrays(GL_TRIANGLES, keyOffset, keyCount);
	}

	// 4. Draw Forcefield Blast Barrier (Clearance feedback pulse on authorization)
	if (hasKeycardObjective && barrierCount > 0) {
		if (!keycard.collected) {
			setMat(lock_ambient, lock_diffuse, lock_specular, barrierLockedMat.material_shininess);
		} else {
			float objNorm = (objectiveFeedbackTimer > 0.0f) ? (objectiveFeedbackTimer / 1.0f) : 0.0f;
			vec4 unlockPulse(0.20f * objNorm, 0.65f * objNorm, 0.80f * objNorm, 0.0f);
			setMat(unlock_ambient + unlockPulse * 0.5f,
			       unlock_diffuse + unlockPulse * 0.8f,
			       unlock_specular + unlockPulse * 1.2f,
			       barrierUnlockedMat.material_shininess + objNorm * 40.0f);
		}
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, view);
		glDrawArrays(GL_TRIANGLES, barrierOffset, barrierCount);
	}

	glBindVertexArray(0);
}
