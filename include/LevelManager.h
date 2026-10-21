#pragma once
#include "Angel/Angel.h"
#include "Materials.h"
#include <vector>
#include <string>

// Axis-Aligned Bounding Box for level obstacles and walls
struct LevelAABB {
	float minX, maxX;
	float minY, maxY;
	float minZ, maxZ;
	int materialType; // 0: hazard crate, 1: structural beam/wall, 2: cyan glow barrier
};

// Movable cargo crate with physics and momentum
struct MovableCrate {
	vec3 pos;
	vec3 halfSize;
	vec3 vel;
	bool active;
};

// Holographic keycard item
struct KeycardItem {
	vec3 pos;
	bool active;
	bool collected;
	float spinAngle;
};

// Detailed score summary per level
struct LevelScoreRecord {
	bool completed = false;
	float timeTaken = 0.0f;
	int basePoints = 1000;
	int timePenalty = 0;       // -10 pts per elapsed second
	int noCollisionBonus = 0;  // +100 pts for 0 collisions
	int noRestartBonus = 0;    // +100 pts for 0 restarts
	int finalScore = 0;        // max(0, 1000 - timePenalty + bonuses)
	int collisions = 0;
	bool restarted = false;
};

class LevelManager {
public:
	enum class GameState {
		PLAYING,
		LEVEL_COMPLETE,
		GAME_COMPLETE,
		OXYGEN_DEPLETED
	};

	LevelManager(GLuint program);
	~LevelManager();

	void loadLevel(int lvl);
	void restartLevel();
	void nextLevel();

	// Progression & State transitions
	void completeCurrentLevel();
	void proceedToNextLevel();
	void replayCurrentLevel();
	void replayGame();
	void recordCollision();

	// Interaction System
	enum InteractResult {
		INTERACT_NONE,
		INTERACT_KEYCARD,
		INTERACT_CRATE,
		INTERACT_ACCESS_DENIED,
		INTERACT_EXIT
	};
	InteractResult tryInteract(const vec4& playerPos, const vec3& facingDir);
	std::string getInteractionPrompt(const vec4& playerPos) const;

	void update(float dt, vec4& playerPos, vec3& playerVel, float hX = 0.45f, float hY = 0.38f, float hZ = 0.90f);
	bool isPlayerSupported(const vec4& playerPos, float hX, float hY, float hZ) const;
	void draw(mat4 view, GLuint program, GLuint ModelView);

	int getCurrentLevel() const { return currentLevel; }
	std::string getLevelName() const { return levelName; }
	const char* getObjectiveText() const { return objectiveText.c_str(); }

	bool isKeycardNeeded() const { return hasKeycardObjective; }
	bool isKeycardCollected() const { return keycard.collected; }

	bool isOxygenActive() const { return oxygenActive; }
	float getOxygenTime() const { return oxygenRemaining; }
	float getElapsedTime() const { return levelElapsedTime; }
	float getTotalGameTime() const { return totalGameTime; }

	vec4 getPlayerSpawn() const { return playerSpawnPos; }
	GameState getState() const { return currentState; }

	// Scoring queries
	const LevelScoreRecord& getCurrentResult() const { return currentResult; }
	const LevelScoreRecord& getRecord(int lvl) const { return levelRecords[lvl - 1]; }
	int getTotalScore() const { return totalScore; }
	float getTotalCompletedTime() const;
	int getCurrentCollisions() const { return currentCollisions; }
	bool isCurrentRestarted() const { return currentRestarted; }
	int getLiveLevelScore() const;

	const std::vector<LevelAABB>& getObstacles() const { return obstacles; }
	const MovableCrate& getCrate() const { return crate; }

	// Visual Feedback & Animation queries (delta-time integrated)
	float getLevelCompleteAnimTimer() const { return levelCompleteAnimTimer; }
	void updateLevelCompleteAnim(float dt) {
		if (currentState == GameState::LEVEL_COMPLETE || currentState == GameState::GAME_COMPLETE) {
			levelCompleteAnimTimer += dt;
		}
	}
	float getLevelIntroTimer() const { return levelIntroTimer; }
	void dismissLevelIntro() { if (levelIntroTimer > 0.5f) levelIntroTimer = 0.5f; }
	void triggerCrateMovedFeedback() { crateMovedFeedbackTimer = 0.50f; }
	void triggerObjectiveFeedback() { objectiveFeedbackTimer = 1.0f; }

private:
	int currentLevel;
	GameState currentState;
	std::string levelName;
	std::string objectiveText;
	vec4 playerSpawnPos;

	float levelElapsedTime;
	float totalGameTime;
	float transitionCooldown;

	// Visual Feedback & Animation timers
	float pulseTimer;
	float crateMovedFeedbackTimer;
	float objectiveFeedbackTimer;
	float levelCompleteAnimTimer;
	float levelIntroTimer;

	// Scoring & Performance metrics
	int currentCollisions;
	float collisionCooldown;
	bool currentRestarted;
	int totalScore;
	LevelScoreRecord currentResult;
	LevelScoreRecord levelRecords[5];

	// Level-specific mechanics
	bool hasKeycardObjective;
	KeycardItem keycard;

	bool hasMovableCrate;
	MovableCrate crate;

	bool oxygenActive;
	float oxygenRemaining;

	std::vector<LevelAABB> obstacles;

	// Exit aperture check
	float exitTriggerY;
	bool isExitBlocked() const;

	// Rendering resources
	GLuint vao;
	GLuint vbo;
	bool bufferDirty;
	GLsizei obsOffset, obsCount;
	GLsizei crateOffset, crateCount;
	GLsizei keyOffset, keyCount;
	GLsizei barrierOffset, barrierCount;

	// Material sets
	Material obstacleMat;
	Material crateMat;
	Material keycardMat;
	Material barrierLockedMat;
	Material barrierUnlockedMat;

	vec4 obs_ambient, obs_diffuse, obs_specular;
	vec4 crate_ambient, crate_diffuse, crate_specular;
	vec4 key_ambient, key_diffuse, key_specular;
	vec4 lock_ambient, lock_diffuse, lock_specular;
	vec4 unlock_ambient, unlock_diffuse, unlock_specular;

	void initMaterials();
	void rebuildGeometry(GLuint program);
	static void addBox(float minX, float minY, float minZ,
	                   float maxX, float maxY, float maxZ,
	                   std::vector<vec4>& pts, std::vector<vec3>& norms);
	static void addQuad(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3,
	                    std::vector<vec4>& pts, std::vector<vec3>& norms);
};
