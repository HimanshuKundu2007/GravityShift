#include "Player.h"
#include "Room.h"
#include <cmath>
#include <iostream>
#include <algorithm>

static const float PI = 3.14159265358979323846f;

// -----------------------------------------------------------------------------
// Procedural Geometry Construction Helpers
// -----------------------------------------------------------------------------

/**
 * @brief Constructs two counter-clockwise triangles forming a flat quad.
 * Calculates planar face normal via cross product of edges.
 */
static void addQuad(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3,
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

/**
 * @brief Constructs a 6-sided 3D Axis-Aligned Box with outward-pointing normals.
 */
static void addBox(float minX, float minY, float minZ,
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

	// Bottom (-Z) & Top (+Z)
	addQuad(v000, v100, v110, v010, pts, norms);
	addQuad(v001, v011, v111, v101, pts, norms);
	// Front (-Y) & Back (+Y)
	addQuad(v000, v001, v101, v100, pts, norms);
	addQuad(v110, v111, v011, v010, pts, norms);
	// Left (-X) & Right (+X)
	addQuad(v000, v010, v011, v001, pts, norms);
	addQuad(v100, v101, v111, v110, pts, norms);
}

/**
 * @brief Constructs a UV sphere using spherical polar coordinates.
 * Generates smooth vertex normals directed outward from center (cx, cy, cz).
 */
static void addSphere(float cx, float cy, float cz, float r, int stacks, int slices,
                      std::vector<vec4>& pts, std::vector<vec3>& norms) {
	for (int i = 0; i < stacks; ++i) {
		float phi1 = -PI / 2.0f + (float)i * (PI / stacks);
		float phi2 = -PI / 2.0f + (float)(i + 1) * (PI / stacks);

		for (int j = 0; j < slices; ++j) {
			float theta1 = (float)j * (2.0f * PI / slices);
			float theta2 = (float)(j + 1) * (2.0f * PI / slices);

			vec4 p1(cx + r * cos(phi1) * sin(theta1), cy + r * cos(phi1) * cos(theta1), cz + r * sin(phi1), 1.0f);
			vec4 p2(cx + r * cos(phi2) * sin(theta1), cy + r * cos(phi2) * sin(theta1), cz + r * sin(phi2), 1.0f);
			vec4 p3(cx + r * cos(phi2) * sin(theta2), cy + r * cos(phi2) * cos(theta2), cz + r * sin(phi2), 1.0f);
			vec4 p4(cx + r * cos(phi1) * sin(theta2), cy + r * cos(phi1) * cos(theta2), cz + r * sin(phi1), 1.0f);

			vec3 n1 = normalize(vec3(p1.x - cx, p1.y - cy, p1.z - cz));
			vec3 n2 = normalize(vec3(p2.x - cx, p2.y - cy, p2.z - cz));
			vec3 n3 = normalize(vec3(p3.x - cx, p3.y - cy, p3.z - cz));
			vec3 n4 = normalize(vec3(p4.x - cx, p4.y - cy, p4.z - cz));

			pts.push_back(p1); norms.push_back(n1);
			pts.push_back(p2); norms.push_back(n2);
			pts.push_back(p3); norms.push_back(n3);
			pts.push_back(p1); norms.push_back(n1);
			pts.push_back(p3); norms.push_back(n3);
			pts.push_back(p4); norms.push_back(n4);
		}
	}
}

/**
 * @brief Constructs an open 3D cylinder aligned along the Z axis.
 */
static void addCylinder(float cx, float cy, float cz, float r, float h, int slices,
                        std::vector<vec4>& pts, std::vector<vec3>& norms) {
	float halfH = h * 0.5f;
	for (int i = 0; i < slices; ++i) {
		float a1 = (float)i * (2.0f * PI / slices);
		float a2 = (float)(i + 1) * (2.0f * PI / slices);

		vec4 p1(cx + r * sin(a1), cy + r * cos(a1), cz - halfH, 1.0f);
		vec4 p2(cx + r * sin(a2), cy + r * cos(a2), cz - halfH, 1.0f);
		vec4 p3(cx + r * sin(a2), cy + r * cos(a2), cz + halfH, 1.0f);
		vec4 p4(cx + r * sin(a1), cy + r * cos(a1), cz + halfH, 1.0f);

		addQuad(p1, p2, p3, p4, pts, norms);
	}
}

// -----------------------------------------------------------------------------
// Player (Articulated 3D Astronaut) Implementation
// -----------------------------------------------------------------------------

Player::Player(vec4 initPosition, GLfloat initHeading, GLfloat initLinearVel, GLfloat initAngularVel, GLuint lifespan)
	: lifespan(lifespan)
	, heading(initHeading)
	, linearVelocity(initLinearVel)
	, angularVelocity(initAngularVel)
	, targetLinearVel(initLinearVel)
	, targetAngularVel(initAngularVel)
	, isCrashed(0)
	, isHitByBeam(0)
	, currentGravity(GRAV_FLOOR)
	, gravityMagnitude(20.0f)
	, walkSpeed(6.8f)
	, jumpSpeed(8.5f)
	, isGrounded(true)
	, yawAngle(0.0f)
	, isBoosting(false)
	, currentSpeed(0.0f)
	, suitPointsCount(0)
	, visorPointsCount(0)
	, jointPointsCount(0)
	, accentPointsCount(0)
	, ringPointsCount(0)
	, corePointsCount(0)
	, gravityCooldown(0.0f)
	, gravityFlashTimer(0.0f)
	, gravityOrientProgress(1.0f)
	, walkAnimPhase(0.0f)
	, walkAnimWeight(0.0f)
	, strideFrequency(0.0f)
{
	position = initPosition;
	spawnPosition = initPosition;
	spawnHeading = initHeading;
	velocity = vec3(0.0f, 0.0f, 0.0f);
	zeroGVelocity = vec3(0.0f, 0.0f, 0.0f);
	gravityVec = vec3(0.0f, 0.0f, -gravityMagnitude);

	setGravity(GRAV_FLOOR);
	surfaceOrientation = targetSurfaceOrientation;
	prevSurfaceOrientation = targetSurfaceOrientation;
	gravityFlashTimer = 0.0f;
	gravityOrientProgress = 1.0f;

	calculateLighting();
	buildAstronautGeometry();
}

Player::~Player() {
	points.clear();
	normals.clear();
}

/**
 * @brief Configures Phong material parameters (Ka, Kd, Ks, alpha) for the astronaut suit.
 */
void Player::calculateLighting() {
	vec4 light_amb(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_diff(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_spec(1.0f, 1.0f, 1.0f, 1.0f);

	// 1. Spacesuit Body (Predominantly clean white / light gray composite)
	suitMat.material_ambient = vec4(0.38f, 0.40f, 0.44f, 1.0f);
	suitMat.material_diffuse = vec4(0.85f, 0.88f, 0.92f, 1.0f);
	suitMat.material_specular = vec4(0.55f, 0.58f, 0.62f, 1.0f);
	suitMat.material_shininess = 35.0f;
	suit_ambient = light_amb * suitMat.material_ambient;
	suit_diffuse = light_diff * suitMat.material_diffuse;
	suit_specular = light_spec * suitMat.material_specular;

	// 2. Visor (Glowing reflective sci-fi cyan/blue glass)
	visorMat.material_ambient = vec4(0.10f, 0.70f, 0.90f, 1.0f);
	visorMat.material_diffuse = vec4(0.20f, 0.85f, 1.00f, 1.0f);
	visorMat.material_specular = vec4(0.95f, 1.00f, 1.00f, 1.0f);
	visorMat.material_shininess = 95.0f;
	visor_ambient = light_amb * visorMat.material_ambient;
	visor_diffuse = light_diff * visorMat.material_diffuse;
	visor_specular = light_spec * visorMat.material_specular;

	// 3. Dark Graphite Structural Components (Joints, Backpack, Boots)
	jointMat.material_ambient = vec4(0.14f, 0.16f, 0.20f, 1.0f);
	jointMat.material_diffuse = vec4(0.24f, 0.26f, 0.30f, 1.0f);
	jointMat.material_specular = vec4(0.50f, 0.55f, 0.60f, 1.0f);
	jointMat.material_shininess = 50.0f;
	joint_ambient = light_amb * jointMat.material_ambient;
	joint_diffuse = light_diff * jointMat.material_diffuse;
	joint_specular = light_spec * jointMat.material_specular;

	// 4. Safety Orange Hazard / Badges / Life-support details
	accentMat.material_ambient = vec4(0.80f, 0.42f, 0.10f, 1.0f);
	accentMat.material_diffuse = vec4(1.00f, 0.58f, 0.18f, 1.0f);
	accentMat.material_specular = vec4(0.70f, 0.60f, 0.45f, 1.0f);
	accentMat.material_shininess = 65.0f;
	accent_ambient = light_amb * accentMat.material_ambient;
	accent_diffuse = light_diff * accentMat.material_diffuse;
	accent_specular = light_spec * accentMat.material_specular;

	// Compatibility aliases
	ambient_product = suit_ambient;
	diffuse_product = suit_diffuse;
	specular_product = suit_specular;
	core_ambient_product = visor_ambient;
	core_diffuse_product = visor_diffuse;
	core_specular_product = visor_specular;
}

/**
 * @brief Constructs articulated 3D astronaut character geometry with distinct limb segments.
 * 
 * Each body part (Torso, Left Arm, Right Arm, Left Thigh, Left Shin, Right Thigh, Right Shin)
 * is recorded into sub-ranges so it can be transformed independently during walking animation.
 */
void Player::buildAstronautGeometry() {
	points.clear();
	normals.clear();

	// Lambda helper to append geometric component and track index sub-range
	auto appendRange = [&](MeshRange& range, const std::vector<vec4>& pts, const std::vector<vec3>& norms) {
		range.start = (GLuint)points.size();
		range.count = (GLuint)pts.size();
		points.insert(points.end(), pts.begin(), pts.end());
		normals.insert(normals.end(), norms.begin(), norms.end());
	};

	// -------------------------------------------------------------
	// 1. SPACESUIT MATERIAL PASS
	// -------------------------------------------------------------
	GLuint suitStart = (GLuint)points.size();

	// A. Torso & Helmet Dome
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addSphere(0.0f, 0.0f, 0.65f, 0.24f, 12, 16, pts, norms);
		addBox(-0.26f, -0.17f, -0.06f, 0.26f, 0.17f, 0.46f, pts, norms);
		appendRange(torsoSuit, pts, norms);
	}

	// B. Left Arm (Upper arm + Forearm)
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.38f, -0.08f, 0.16f, -0.27f, 0.08f, 0.40f, pts, norms);
		addBox(-0.37f, -0.07f, -0.06f, -0.28f, 0.07f, 0.14f, pts, norms);
		appendRange(leftArmSuit, pts, norms);
	}

	// C. Right Arm (Upper arm + Forearm)
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.27f, -0.08f, 0.16f,  0.38f, 0.08f, 0.40f, pts, norms);
		addBox( 0.28f, -0.07f, -0.06f,  0.37f, 0.07f, 0.14f, pts, norms);
		appendRange(rightArmSuit, pts, norms);
	}

	// D. Left Thigh
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.21f, -0.10f, -0.46f, -0.07f, 0.10f, -0.07f, pts, norms);
		appendRange(leftThighSuit, pts, norms);
	}

	// E. Left Shin
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.20f, -0.09f, -0.74f, -0.08f, 0.09f, -0.46f, pts, norms);
		appendRange(leftShinSuit, pts, norms);
	}

	// F. Right Thigh
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.07f, -0.10f, -0.46f,  0.21f, 0.10f, -0.07f, pts, norms);
		appendRange(rightThighSuit, pts, norms);
	}

	// G. Right Shin
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.08f, -0.09f, -0.74f,  0.20f, 0.09f, -0.46f, pts, norms);
		appendRange(rightShinSuit, pts, norms);
	}
	suitPointsCount = (GLuint)points.size() - suitStart;

	// -------------------------------------------------------------
	// 2. VISOR MATERIAL PASS (Reflective Cyan Glass Faceplate)
	// -------------------------------------------------------------
	GLuint visorStart = (GLuint)points.size();
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.16f, 0.10f, 0.54f, 0.16f, 0.25f, 0.74f, pts, norms);
		addSphere(0.0f, 0.12f, 0.65f, 0.18f, 8, 12, pts, norms);
		appendRange(torsoVisor, pts, norms);
	}
	visorPointsCount = (GLuint)points.size() - visorStart;

	// -------------------------------------------------------------
	// 3. JOINTS, BACKPACK & BOOTS MATERIAL PASS (Dark Graphite Steel)
	// -------------------------------------------------------------
	GLuint jointStart = (GLuint)points.size();

	// A. Torso Joints: Life Support Backpack & Twin Oxygen Cylinders
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.22f, -0.34f, -0.02f, 0.22f, -0.17f, 0.44f, pts, norms);
		addCylinder(-0.12f, -0.36f, 0.21f, 0.065f, 0.36f, 10, pts, norms);
		addCylinder( 0.12f, -0.36f, 0.21f, 0.065f, 0.36f, 10, pts, norms);
		appendRange(torsoJoint, pts, norms);
	}

	// B. Left Arm Joints: Shoulder seal, Elbow seal, Glove
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addSphere(-0.31f, 0.0f, 0.40f, 0.075f, 6, 8, pts, norms);
		addSphere(-0.33f, 0.0f, 0.15f, 0.065f, 6, 8, pts, norms);
		addBox(-0.38f, -0.08f, -0.18f, -0.27f, 0.09f, -0.06f, pts, norms);
		appendRange(leftArmJoint, pts, norms);
	}

	// C. Right Arm Joints: Shoulder seal, Elbow seal, Glove
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addSphere( 0.31f, 0.0f, 0.40f, 0.075f, 6, 8, pts, norms);
		addSphere( 0.33f, 0.0f, 0.15f, 0.065f, 6, 8, pts, norms);
		addBox( 0.27f, -0.08f, -0.18f,  0.38f, 0.09f, -0.06f, pts, norms);
		appendRange(rightArmJoint, pts, norms);
	}

	// D. Left Knee Joint
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addSphere(-0.14f, 0.0f, -0.46f, 0.075f, 6, 8, pts, norms);
		appendRange(leftThighJoint, pts, norms);
	}

	// E. Left Boot
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.22f, -0.12f, -0.90f, -0.06f, 0.18f, -0.74f, pts, norms);
		appendRange(leftShinJoint, pts, norms);
	}

	// F. Right Knee Joint
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addSphere( 0.14f, 0.0f, -0.46f, 0.075f, 6, 8, pts, norms);
		appendRange(rightThighJoint, pts, norms);
	}

	// G. Right Boot
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.06f, -0.12f, -0.90f,  0.22f, 0.18f, -0.74f, pts, norms);
		appendRange(rightShinJoint, pts, norms);
	}
	jointPointsCount = (GLuint)points.size() - jointStart;

	// -------------------------------------------------------------
	// 4. ACCENTS MATERIAL PASS (Safety Orange Badges & Stripes)
	// -------------------------------------------------------------
	GLuint accentStart = (GLuint)points.size();

	// A. Torso Accent: Chest Life Support Console
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.14f, 0.17f, 0.10f, 0.14f, 0.22f, 0.32f, pts, norms);
		appendRange(torsoAccent, pts, norms);
	}

	// B. Left Arm Accent Stripe
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.38f, -0.085f, 0.26f, -0.265f, 0.085f, 0.30f, pts, norms);
		appendRange(leftArmAccent, pts, norms);
	}

	// C. Right Arm Accent Stripe
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.265f, -0.085f, 0.26f,  0.38f, 0.085f, 0.30f, pts, norms);
		appendRange(rightArmAccent, pts, norms);
	}

	// D. Left Thigh Stripe
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox(-0.215f, -0.105f, -0.28f, -0.065f, 0.105f, -0.24f, pts, norms);
		appendRange(leftThighAccent, pts, norms);
	}

	// E. Right Thigh Stripe
	{
		std::vector<vec4> pts; std::vector<vec3> norms;
		addBox( 0.065f, -0.105f, -0.28f,  0.215f, 0.105f, -0.24f, pts, norms);
		appendRange(rightThighAccent, pts, norms);
	}
	accentPointsCount = (GLuint)points.size() - accentStart;

	ringPointsCount = suitPointsCount;
	corePointsCount = visorPointsCount + jointPointsCount + accentPointsCount;
}

/**
 * @brief Renders the articulated astronaut with hierarchical limb walking animation
 * and multi-material Phong illumination.
 * 
 * Hierarchical Pipeline:
 * 1. Base Matrix: Translate(pos) * surfaceOrientation * RotateZ(yawAngle)
 * 2. Torso Matrix: Base * Translate(0, 0, zBounce) * RotateX(torsoPitch) * RotateY(torsoRoll)
 * 3. Left / Right Hips rotate around hip pivots via RotateX(legSwing)
 * 4. Left / Right Knees bend backward relative to thighs via RotateX(-kneeBend)
 * 5. Left / Right Shoulders swing in opposition via RotateX(armSwing)
 */
void Player::draw(mat4 view, GLuint program, GLuint modelView_ptr) {
	// Base character orientation in world space
	mat4 baseModel = Translate(position) * surfaceOrientation * RotateZ(yawAngle);

	// -------------------------------------------------------------
	// 1. Calculate Articulated Kinematic Joint Angles
	// -------------------------------------------------------------
	// Alternating leg swing (+36 deg forward, -36 deg backward) for a bold, clear stride
	float legSwing = std::sin(walkAnimPhase) * 36.0f * walkAnimWeight;

	// Opposing arm swing (+30 deg forward, -30 deg backward)
	float armSwing = -std::sin(walkAnimPhase) * 30.0f * walkAnimWeight;

	// Pronounced knee flexion and boot lift:
	// When a leg swings backward and recovers forward, the knee flexes up to 52 degrees,
	// lifting the boot clearly off the ground and angling the sole backward toward the camera.
	float leftKneeBend = 0.0f;
	float rightKneeBend = 0.0f;
	if (walkAnimWeight > 0.01f) {
		float lSine = std::sin(walkAnimPhase);
		if (lSine < 0.0f) {
			leftKneeBend = (-lSine) * 52.0f * walkAnimWeight;
		}
		float rSine = -std::sin(walkAnimPhase);
		if (rSine < 0.0f) {
			rightKneeBend = (-rSine) * 52.0f * walkAnimWeight;
		}
	}

	// In-air leap / falling posture when not grounded
	if (!isGrounded) {
		float jumpFactor = 1.0f - walkAnimWeight;
		legSwing += (jumpFactor * -8.0f);
		armSwing += (jumpFactor * 16.0f);
		leftKneeBend += (jumpFactor * 18.0f);
		rightKneeBend += (jumpFactor * 12.0f);
	}

	// Dynamic body bounce and lateral weight shift:
	// - Vertical bounce: drops and rises twice per full stride cycle (clearly visible backpack bob)
	float zBounce = -std::abs(std::sin(walkAnimPhase)) * 0.060f * walkAnimWeight;
	// - Lateral pelvic sway: shifts weight over the stance foot
	float xSway = std::sin(walkAnimPhase) * 0.045f * walkAnimWeight;
	// - Torso roll: side-to-side rhythmic tilt
	float torsoRoll = std::sin(walkAnimPhase) * 3.5f * walkAnimWeight;
	// - Torso pitch: athletic forward lean
	float torsoPitch = walkAnimWeight * 3.0f;

	// Root Torso Model Matrix (incorporates bounce, sway, pitch, roll)
	mat4 modelTorso = baseModel * Translate(xSway, 0.0f, zBounce) * RotateX(torsoPitch) * RotateY(torsoRoll);
	model = modelTorso;

	// Biological Joint Pivots in Character Local Coordinates
	const vec3 leftShoulderPivot(-0.31f, 0.0f, 0.38f);
	const vec3 rightShoulderPivot(0.31f, 0.0f, 0.38f);
	const vec3 leftHipPivot(-0.14f, 0.0f, -0.07f);
	const vec3 rightHipPivot(0.14f, 0.0f, -0.07f);
	const vec3 leftKneePivot(-0.14f, 0.0f, -0.46f);
	const vec3 rightKneePivot(0.14f, 0.0f, -0.46f);

	// Arm lateral outward flare (10 deg) so arms swing cleanly outside the backpack silhouette
	const float armFlare = 10.0f;

	// Hierarchical Limbs Model Matrices
	mat4 modelLeftArm = modelTorso
		* Translate(leftShoulderPivot)
		* RotateY(-armFlare)
		* RotateX(armSwing)
		* Translate(-leftShoulderPivot);

	mat4 modelRightArm = modelTorso
		* Translate(rightShoulderPivot)
		* RotateY(armFlare)
		* RotateX(-armSwing)
		* Translate(-rightShoulderPivot);

	mat4 modelLeftThigh = modelTorso
		* Translate(leftHipPivot)
		* RotateX(legSwing)
		* Translate(-leftHipPivot);

	mat4 modelLeftLowerLeg = modelLeftThigh
		* Translate(leftKneePivot)
		* RotateX(-leftKneeBend)
		* Translate(-leftKneePivot);

	mat4 modelRightThigh = modelTorso
		* Translate(rightHipPivot)
		* RotateX(-legSwing)
		* Translate(-rightHipPivot);

	mat4 modelRightLowerLeg = modelRightThigh
		* Translate(rightKneePivot)
		* RotateX(-rightKneeBend)
		* Translate(-rightKneePivot);

	// Gravity-shift visual pulse factor
	float flashFactor = 0.0f;
	if (gravityFlashTimer > 0.0f) {
		float norm = gravityFlashTimer / 0.35f;
		flashFactor = std::sin(norm * 3.14159f);
	}
	vec4 cyanFlash(0.18f * flashFactor, 0.65f * flashFactor, 0.95f * flashFactor, 0.0f);

	// Helper to draw a mesh sub-range with an active hierarchical model-view transform
	auto drawRange = [&](const MeshRange& range, const mat4& partModel) {
		if (range.count == 0) return;
		glUniformMatrix4fv(modelView_ptr, 1, GL_TRUE, view * partModel);
		glDrawArrays(GL_TRIANGLES, range.start, range.count);
	};

	// -------------------------------------------------------------
	// Pass 1: Spacesuit Body (Diffuse White + Cyan Highlight)
	// -------------------------------------------------------------
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, suit_ambient + cyanFlash * 0.4f);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, suit_diffuse + cyanFlash * 0.8f);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, suit_specular + cyanFlash * 0.6f);
	glUniform1f(glGetUniformLocation(program, "Shininess"), suitMat.material_shininess + flashFactor * 30.0f);

	drawRange(torsoSuit, modelTorso);
	drawRange(leftArmSuit, modelLeftArm);
	drawRange(rightArmSuit, modelRightArm);
	drawRange(leftThighSuit, modelLeftThigh);
	drawRange(leftShinSuit, modelLeftLowerLeg);
	drawRange(rightThighSuit, modelRightThigh);
	drawRange(rightShinSuit, modelRightLowerLeg);

	// -------------------------------------------------------------
	// Pass 2: Visor (Reflective Cyan Glass Faceplate)
	// -------------------------------------------------------------
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, visor_ambient + cyanFlash * 0.5f);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, visor_diffuse + cyanFlash * 0.8f);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, visor_specular + cyanFlash);
	glUniform1f(glGetUniformLocation(program, "Shininess"), visorMat.material_shininess);

	drawRange(torsoVisor, modelTorso);

	// -------------------------------------------------------------
	// Pass 3: Joints, Backpack & Boots (Dark Graphite Steel)
	// -------------------------------------------------------------
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, joint_ambient);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, joint_diffuse);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, joint_specular);
	glUniform1f(glGetUniformLocation(program, "Shininess"), jointMat.material_shininess);

	drawRange(torsoJoint, modelTorso);
	drawRange(leftArmJoint, modelLeftArm);
	drawRange(rightArmJoint, modelRightArm);
	drawRange(leftThighJoint, modelLeftThigh);
	drawRange(leftShinJoint, modelLeftLowerLeg);
	drawRange(rightThighJoint, modelRightThigh);
	drawRange(rightShinJoint, modelRightLowerLeg);

	// -------------------------------------------------------------
	// Pass 4: Hazard Badges & Accents (Safety Orange)
	// -------------------------------------------------------------
	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, accent_ambient);
	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, accent_diffuse);
	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, accent_specular);
	glUniform1f(glGetUniformLocation(program, "Shininess"), accentMat.material_shininess);

	drawRange(torsoAccent, modelTorso);
	drawRange(leftArmAccent, modelLeftArm);
	drawRange(rightArmAccent, modelRightArm);
	drawRange(leftThighAccent, modelLeftThigh);
	drawRange(rightThighAccent, modelRightThigh);
}

void Player::cycleGravity() {
	if (gravityCooldown > 0.0f) return;
	gravityCooldown = 0.30f;
	currentGravity = (GravityDir)((currentGravity + 1) % 4);
	setGravity(currentGravity);
}

/**
 * @brief Sets active gravity direction, configuring gravity acceleration vector
 * and target surface orientation matrix.
 */
void Player::setGravity(GravityDir dir) {
	currentGravity = dir;
	isGrounded = false; // Disengage from previous surface

	prevSurfaceOrientation = surfaceOrientation;
	gravityOrientProgress = 0.0f;
	gravityFlashTimer = 0.35f;

	switch (currentGravity) {
	case GRAV_FLOOR:
		gravityVec = vec3(0.0f, 0.0f, -gravityMagnitude);
		targetSurfaceOrientation = mat4(); // Identity
		break;
	case GRAV_LEFT_WALL:
		gravityVec = vec3(-gravityMagnitude, 0.0f, 0.0f);
		targetSurfaceOrientation = RotateY(90.0f);
		break;
	case GRAV_CEILING:
		gravityVec = vec3(0.0f, 0.0f, gravityMagnitude);
		targetSurfaceOrientation = RotateX(180.0f);
		break;
	case GRAV_RIGHT_WALL:
		gravityVec = vec3(gravityMagnitude, 0.0f, 0.0f);
		targetSurfaceOrientation = RotateY(-90.0f);
		break;
	}
}

vec3 Player::getUpVector() const {
	switch (currentGravity) {
	case GRAV_FLOOR:      return vec3(0.0f, 0.0f, 1.0f);
	case GRAV_LEFT_WALL:  return vec3(1.0f, 0.0f, 0.0f);
	case GRAV_CEILING:    return vec3(0.0f, 0.0f, -1.0f);
	case GRAV_RIGHT_WALL: return vec3(-1.0f, 0.0f, 0.0f);
	}
	return vec3(0.0f, 0.0f, 1.0f);
}

vec3 Player::getDownVector() const {
	return -getUpVector();
}

vec3 Player::getNormalVector() const {
	return getUpVector();
}

const char* Player::getGravityName() const {
	switch (currentGravity) {
	case GRAV_FLOOR:      return "FLOOR";
	case GRAV_LEFT_WALL:  return "LEFT WALL";
	case GRAV_CEILING:    return "CEILING";
	case GRAV_RIGHT_WALL: return "RIGHT WALL";
	}
	return "UNKNOWN";
}

const char* Player::getGravityArrow() const {
	switch (currentGravity) {
	case GRAV_FLOOR:      return "[ v ]";
	case GRAV_LEFT_WALL:  return "[ < ]";
	case GRAV_CEILING:    return "[ ^ ]";
	case GRAV_RIGHT_WALL: return "[ > ]";
	}
	return "[ - ]";
}

vec3 Player::getFacingVector() const {
	float rad = yawAngle * (PI / 180.0f);
	vec4 localFacing(-sin(rad), cos(rad), 0.0f, 0.0f);
	vec4 worldFacing = surfaceOrientation * localFacing;
	return normalize(vec3(worldFacing.x, worldFacing.y, worldFacing.z));
}

/**
 * @brief Returns active Axis-Aligned Bounding Box (AABB) half-extents aligned to world coordinates.
 */
void Player::getEffectiveHalfExtents(float& hx, float& hy, float& hz) const {
	if (currentGravity == GRAV_FLOOR || currentGravity == GRAV_CEILING) {
		hx = HALF_X; // 0.45f
		hy = HALF_Y; // 0.38f
		hz = HALF_Z; // 0.90f
	} else {
		// On left or right walls, height is along world X, width along world Z
		hx = HALF_Z; // 0.90f
		hy = HALF_Y; // 0.38f
		hz = HALF_X; // 0.45f
	}
}

/**
 * @brief Kinematic walking integration relative to active surface tangent vectors.
 */
void Player::walk(float fwdInput, float strafeInput, float dt) {
	vec3 tFwd(0.0f, 0.0f, 0.0f);
	vec3 tStrafe(0.0f, 0.0f, 0.0f);

	switch (currentGravity) {
	case GRAV_FLOOR:
		tFwd = vec3(0.0f, 1.0f, 0.0f);
		tStrafe = vec3(1.0f, 0.0f, 0.0f);
		break;
	case GRAV_LEFT_WALL:
		tFwd = vec3(0.0f, 1.0f, 0.0f);
		tStrafe = vec3(0.0f, 0.0f, 1.0f);
		break;
	case GRAV_CEILING:
		tFwd = vec3(0.0f, 1.0f, 0.0f);
		tStrafe = vec3(-1.0f, 0.0f, 0.0f);
		break;
	case GRAV_RIGHT_WALL:
		tFwd = vec3(0.0f, 1.0f, 0.0f);
		tStrafe = vec3(0.0f, 0.0f, -1.0f);
		break;
	}

	vec3 targetSurfaceVel = tFwd * (fwdInput * walkSpeed) + tStrafe * (strafeInput * walkSpeed);
	bool hasInput = (std::abs(fwdInput) > 0.01f || std::abs(strafeInput) > 0.01f);

	// Update yaw heading to face movement direction
	if (hasInput) {
		float desiredYaw = 0.0f;
		if (fwdInput > 0.0f && std::abs(strafeInput) < 0.1f) desiredYaw = 0.0f;
		else if (fwdInput < 0.0f && std::abs(strafeInput) < 0.1f) desiredYaw = 180.0f;
		else if (strafeInput > 0.0f && std::abs(fwdInput) < 0.1f) desiredYaw = 90.0f;
		else if (strafeInput < 0.0f && std::abs(fwdInput) < 0.1f) desiredYaw = -90.0f;
		else if (fwdInput > 0.0f && strafeInput > 0.0f) desiredYaw = 45.0f;
		else if (fwdInput > 0.0f && strafeInput < 0.0f) desiredYaw = -45.0f;
		else if (fwdInput < 0.0f && strafeInput > 0.0f) desiredYaw = 135.0f;
		else if (fwdInput < 0.0f && strafeInput < 0.0f) desiredYaw = -135.0f;
		yawAngle = desiredYaw;
	}

	// Tangential velocity integration with friction
	vec3 up = getUpVector();
	vec3 normalVel = up * dot(velocity, up);
	vec3 currentSurfaceVel = velocity - normalVel;

	if (hasInput) {
		const float accelRate = isGrounded ? 38.0f : 14.0f;
		vec3 diff = targetSurfaceVel - currentSurfaceVel;
		currentSurfaceVel += diff * (accelRate * dt);
	} else {
		// Coulomb surface friction deceleration to rest
		const float friction = isGrounded ? 18.0f : 4.0f;
		float damp = 1.0f - (friction * dt);
		if (damp < 0.0f) damp = 0.0f;
		currentSurfaceVel *= damp;
		if (length(currentSurfaceVel) < 0.05f) {
			currentSurfaceVel = vec3(0.0f, 0.0f, 0.0f);
		}
	}

	velocity = currentSurfaceVel + normalVel;
}

void Player::jump() {
	if (isGrounded) {
		vec3 up = getUpVector();
		float currNormalVel = dot(velocity, up);
		if (currNormalVel < 0.0f) {
			velocity -= up * currNormalVel;
		}
		velocity += up * jumpSpeed;
		isGrounded = false;
	}
}

/**
 * @brief Performs numerical integration of player physics (gravity, velocity, position)
 * and advances the dynamic walking animation cycle.
 */
void Player::updatePhysics(float dt) {
	if (gravityCooldown > 0.0f) {
		gravityCooldown -= dt;
		if (gravityCooldown < 0.0f) gravityCooldown = 0.0f;
	}

	if (gravityFlashTimer > 0.0f) {
		gravityFlashTimer -= dt;
		if (gravityFlashTimer < 0.0f) gravityFlashTimer = 0.0f;
	}

	// Smooth surface orientation transition (cubic smoothstep)
	if (gravityOrientProgress < 1.0f) {
		gravityOrientProgress += dt / 0.18f;
		if (gravityOrientProgress >= 1.0f) {
			gravityOrientProgress = 1.0f;
			surfaceOrientation = targetSurfaceOrientation;
		} else {
			float t = gravityOrientProgress;
			float smoothT = t * t * (3.0f - 2.0f * t);
			mat4 interp;
			for (int r = 0; r < 4; ++r) {
				for (int c = 0; c < 4; ++c) {
					interp[r][c] = prevSurfaceOrientation[r][c] * (1.0f - smoothT) + targetSurfaceOrientation[r][c] * smoothT;
				}
			}
			vec3 col0(interp[0][0], interp[1][0], interp[2][0]);
			vec3 col1(interp[0][1], interp[1][1], interp[2][1]);
			vec3 col2(interp[0][2], interp[1][2], interp[2][2]);
			if (length(col0) > 0.001f) col0 = normalize(col0);
			if (length(col1) > 0.001f) col1 = normalize(col1);
			if (length(col2) > 0.001f) col2 = normalize(col2);
			interp[0][0] = col0.x; interp[0][1] = col1.x; interp[0][2] = col2.x;
			interp[1][0] = col0.y; interp[1][1] = col1.y; interp[1][2] = col2.y;
			interp[2][0] = col0.z; interp[2][1] = col1.z; interp[2][2] = col2.z;
			surfaceOrientation = interp;
		}
	} else {
		surfaceOrientation = targetSurfaceOrientation;
	}

	// Semi-implicit Euler integration: v += g*dt, p += v*dt
	velocity += gravityVec * dt;
	position.x += velocity.x * dt;
	position.y += velocity.y * dt;
	position.z += velocity.z * dt;

	currentSpeed = length(velocity);
	linearVelocity = currentSpeed;
	zeroGVelocity = velocity;

	// -------------------------------------------------------------
	// Synchronize Walking Animation Cycle with Surface Motion
	// -------------------------------------------------------------
	vec3 up = getUpVector();
	vec3 normalVel = up * dot(velocity, up);
	vec3 surfaceVel = velocity - normalVel;
	float speedOnSurface = length(surfaceVel);

	// Check if character is actively moving along surface
	bool isWalkingOnSurface = (speedOnSurface > 0.12f);

	if (isWalkingOnSurface) {
		// Fast, responsive ramp-up so the walking animation begins immediately
		walkAnimWeight += dt * 10.0f;
		if (walkAnimWeight > 1.0f) walkAnimWeight = 1.0f;

		// Stride cycle frequency scales dynamically with travel speed (cadence ~2.4 - 3.2 steps/sec)
		strideFrequency = 7.5f + 3.0f * (speedOnSurface / walkSpeed);
		walkAnimPhase += strideFrequency * dt;
		if (walkAnimPhase > 2.0f * PI) {
			walkAnimPhase -= 2.0f * PI;
		}
	} else {
		// Smooth deceleration to stable neutral idle pose
		walkAnimWeight -= dt * 7.0f;
		if (walkAnimWeight <= 0.0f) {
			walkAnimWeight = 0.0f;
			walkAnimPhase = 0.0f;
			strideFrequency = 0.0f;
		}
	}

	// Model transform synchronization
	model = Translate(position) * surfaceOrientation * RotateZ(yawAngle);

	// Camera anchor points
	TPCameraAttachPoint = position - vec4(0.0f, 5.8f, 0.0f, 0.0f) + vec4(getUpVector() * 1.8f, 0.0f);
	FPCameraAttachPoint = position + vec4(getUpVector() * 0.65f, 0.0f);
}

void Player::applyZeroGThrust(float tx, float ty, float tz, bool boost, float dt) {
	walk(ty, tx, dt);
	if (tz > 0.0f) jump();
}

void Player::brake(float dt) {
	float damp = 1.0f - (22.0f * dt);
	if (damp < 0.0f) damp = 0.0f;
	velocity *= damp;
	zeroGVelocity = velocity;
	currentSpeed = length(velocity);
}

void Player::resetToSpawn() {
	position = spawnPosition;
	heading = spawnHeading;
	yawAngle = 0.0f;
	velocity = vec3(0.0f, 0.0f, 0.0f);
	zeroGVelocity = vec3(0.0f, 0.0f, 0.0f);
	currentSpeed = 0.0f;
	linearVelocity = 0.0f;
	angularVelocity = 0.0f;
	gravityCooldown = 0.0f;
	gravityFlashTimer = 0.0f;
	gravityOrientProgress = 1.0f;
	walkAnimPhase = 0.0f;
	walkAnimWeight = 0.0f;
	strideFrequency = 0.0f;
	isBoosting = false;
	isCrashed = 0;
	setGravity(GRAV_FLOOR);
	surfaceOrientation = targetSurfaceOrientation;
	prevSurfaceOrientation = targetSurfaceOrientation;
	isGrounded = true;
	model = Translate(position) * surfaceOrientation * RotateZ(yawAngle);
}

void Player::move() {
	updatePhysics(1.0f / 60.0f);
}
