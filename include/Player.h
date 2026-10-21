#pragma once
#include "Materials.h"
#include "Angel/Angel.h"
#include <vector>

/**
 * @brief Player character (Astronaut) controller for Gravity Shift.
 * 
 * Manages 3D procedural articulated astronaut mesh geometry, multi-material Phong shading,
 * hierarchical limb transformations, dynamic walking animations, local-to-world model transformations,
 * gravity vector switching, surface orientation interpolation, AABB collision boundaries,
 * and kinematic walking physics.
 */
class Player {
public:
	/// Supported cardinal gravity directions
	enum GravityDir {
		GRAV_FLOOR = 0,     // Down (-Z): Feet on Floor (Z = 0)
		GRAV_LEFT_WALL = 1, // Down (-X): Feet on Left Wall (X = -10)
		GRAV_CEILING = 2,   // Down (+Z): Feet on Ceiling (Z = 8)
		GRAV_RIGHT_WALL = 3 // Down (+X): Feet on Right Wall (X = 10)
	};

	/// Mesh sub-range for articulated hierarchical rendering
	struct MeshRange {
		GLuint start = 0;
		GLuint count = 0;
	};

	Player(vec4 initPosition, GLfloat initHeading, GLfloat initLinearVel, GLfloat initAngularVel, GLuint lifespan);
	~Player();

	/**
	 * @brief Renders the articulated astronaut with hierarchical limb walking animation
	 * and multi-material Phong illumination.
	 * @param view Camera view matrix (World -> Eye coordinates).
	 * @param program Compiled OpenGL shader program.
	 * @param modelView_ptr Uniform location for ModelView matrix.
	 */
	void draw(mat4 view, GLuint program, GLuint modelView_ptr);
	void move();

	// 3D Astronaut Geometry Containers
	std::vector<vec4> points;
	std::vector<vec3> normals;

	// World Transform & State
	vec4 position;
	vec4 spawnPosition;
	GLfloat spawnHeading;
	GLfloat heading;
	GLuint lifespan;
	GLint isCrashed;
	GLint isHitByBeam;

	// Physics & Movement State
	GravityDir currentGravity;
	vec3 gravityVec;
	vec3 velocity;
	vec3 zeroGVelocity;
	bool isGrounded;
	float walkSpeed;
	float jumpSpeed;
	float gravityMagnitude;
	float currentSpeed;
	GLfloat linearVelocity;
	GLfloat angularVelocity;
	GLfloat targetLinearVel;
	GLfloat targetAngularVel;
	bool isBoosting;

	// Surface Orientation
	float yawAngle;
	mat4 model;
	mat4 surfaceOrientation;

	// Camera attachment points
	vec4 FPCameraAttachPoint;
	vec4 TPCameraAttachPoint;

	float gravityCooldown;
	float gravityFlashTimer;
	float gravityOrientProgress;
	mat4 prevSurfaceOrientation;
	mat4 targetSurfaceOrientation;

	// Walking Animation State
	float walkAnimPhase;   ///< Walking stride cycle phase in radians [0 .. 2*PI]
	float walkAnimWeight;  ///< Smooth blending factor between idle (0.0) and full walk (1.0)
	float strideFrequency; ///< Current walking stride frequency in radians/sec

	// Gravity Shift & Controls
	void cycleGravity();
	void setGravity(GravityDir dir);
	void walk(float fwdInput, float strafeInput, float dt);
	void jump();
	void updatePhysics(float dt);
	void updateZeroG(float dt) { updatePhysics(dt); }
	void applyZeroGThrust(float tx, float ty, float tz, bool boost, float dt);
	void brake(float dt);
	void resetToSpawn();

	// Coordinate Queries
	vec3 getUpVector() const;
	vec3 getDownVector() const;
	vec3 getNormalVector() const;
	vec3 getFacingVector() const;
	void getEffectiveHalfExtents(float& hx, float& hy, float& hz) const;
	const char* getGravityName() const;
	const char* getGravityArrow() const;

	// Draw Counts for multi-material astronaut rendering
	GLuint suitPointsCount;
	GLuint visorPointsCount;
	GLuint jointPointsCount;
	GLuint accentPointsCount;

	// Articulated Mesh Ranges for Hierarchical Rendering
	MeshRange torsoSuit;
	MeshRange leftArmSuit;
	MeshRange rightArmSuit;
	MeshRange leftThighSuit;
	MeshRange leftShinSuit;
	MeshRange rightThighSuit;
	MeshRange rightShinSuit;

	MeshRange torsoVisor;

	MeshRange torsoJoint;
	MeshRange leftArmJoint;
	MeshRange rightArmJoint;
	MeshRange leftThighJoint;
	MeshRange leftShinJoint;
	MeshRange rightThighJoint;
	MeshRange rightShinJoint;

	MeshRange torsoAccent;
	MeshRange leftArmAccent;
	MeshRange rightArmAccent;
	MeshRange leftThighAccent;
	MeshRange rightThighAccent;

	// Half-extents for AABB collision detection (total height: 1.80m)
	static constexpr float HALF_X = 0.45f;
	static constexpr float HALF_Y = 0.38f;
	static constexpr float HALF_Z = 0.90f;

private:
	void calculateLighting();
	void buildAstronautGeometry();

	// Material sets
	Material suitMat;
	Material visorMat;
	Material jointMat;
	Material accentMat;

	vec4 suit_ambient, suit_diffuse, suit_specular;
	vec4 visor_ambient, visor_diffuse, visor_specular;
	vec4 joint_ambient, joint_diffuse, joint_specular;
	vec4 accent_ambient, accent_diffuse, accent_specular;

	Material mat;
	Material coreMat;
	vec4 ambient_product, diffuse_product, specular_product;
	vec4 core_ambient_product, core_diffuse_product, core_specular_product;
	GLuint ringPointsCount;
	GLuint corePointsCount;
};

/// Type alias for backward compatibility with existing codebase
using SpaceShip = Player;
