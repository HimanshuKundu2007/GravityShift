#pragma once
#include "Angel/Angel.h"
#include "Materials.h"
#include <vector>

struct CrateBox {
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
};

class Room {
public:
	Room(GLuint program);
	~Room();

	void draw(mat4 view, GLuint program, GLuint modelView_ptr);
	void update(float dt) { doorPulseTimer += dt; }
	void resetEffects() { doorPulseTimer = 0.0f; }
	void toggleCeiling() { showCeiling = !showCeiling; }
	bool isCeilingVisible() const { return showCeiling; }

	// Room bounding dimensions (world units)
	static constexpr float MIN_X = -10.0f;
	static constexpr float MAX_X =  10.0f;
	static constexpr float MIN_Y = -22.0f;
	static constexpr float MAX_Y =  24.0f;
	static constexpr float MIN_Z =   0.0f;
	static constexpr float MAX_Z =   8.0f;

	const std::vector<CrateBox>& getCrates() const { return crates; }

private:
	bool showCeiling;
	float doorPulseTimer;

	// OpenGL buffer objects
	GLuint vao;
	GLuint vbo;

	std::vector<CrateBox> crates;

	// Draw counts and offsets for distinct material batches
	GLsizei floorOffset, floorCount;
	GLsizei wallOffset, wallCount;
	GLsizei beamOffset, beamCount;
	GLsizei cyanLightOffset, cyanLightCount;
	GLsizei warningOffset, warningCount;
	GLsizei doorFrameOffset, doorFrameCount;
	GLsizei doorPanelOffset, doorPanelCount;
	GLsizei crateOffset, crateCount;
	GLsizei ceilingOffset, ceilingCount;

	// Material properties for space station interior
	Material floorMat;
	Material wallMat;
	Material beamMat;
	Material cyanLightMat;
	Material warningMat;
	Material doorFrameMat;
	Material doorPanelMat;
	Material crateMat;
	Material ceilingMat;

	vec4 floor_ambient, floor_diffuse, floor_specular;
	vec4 wall_ambient, wall_diffuse, wall_specular;
	vec4 beam_ambient, beam_diffuse, beam_specular;
	vec4 cyan_ambient, cyan_diffuse, cyan_specular;
	vec4 warn_ambient, warn_diffuse, warn_specular;
	vec4 doorFrame_ambient, doorFrame_diffuse, doorFrame_specular;
	vec4 doorPanel_ambient, doorPanel_diffuse, doorPanel_specular;
	vec4 crate_ambient, crate_diffuse, crate_specular;
	vec4 ceiling_ambient, ceiling_diffuse, ceiling_specular;

	void initMaterials();
	void buildGeometry(GLuint program);

	// Geometry construction helpers
	static void addBox(float minX, float minY, float minZ,
	                   float maxX, float maxY, float maxZ,
	                   std::vector<vec4>& pts, std::vector<vec3>& norms);

	static void addQuad(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3,
	                    std::vector<vec4>& pts, std::vector<vec3>& norms);

	void addCrate(float minX, float minY, float minZ,
	              float maxX, float maxY, float maxZ,
	              std::vector<vec4>& pts, std::vector<vec3>& norms,
	              std::vector<vec4>& beamPts, std::vector<vec3>& beamNorms,
	              std::vector<vec4>& warnPts, std::vector<vec3>& warnNorms);
};
