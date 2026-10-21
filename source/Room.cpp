#include "Room.h"
#include <iostream>

Room::Room(GLuint program) : showCeiling(true), doorPulseTimer(0.0f) {
	initMaterials();
	buildGeometry(program);
}

Room::~Room() {
	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

void Room::initMaterials() {
	// Base lighting reference
	vec4 light_ambient(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_diffuse(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 light_specular(1.0f, 1.0f, 1.0f, 1.0f);

	// 1. Floor: Metallic Silver-Blue Modular Decking with Specular Sheen
	floorMat.material_ambient = vec4(0.20f, 0.22f, 0.26f, 1.0f);
	floorMat.material_diffuse = vec4(0.48f, 0.52f, 0.58f, 1.0f);
	floorMat.material_specular = vec4(0.55f, 0.60f, 0.68f, 1.0f);
	floorMat.material_shininess = 50.0f;
	floor_ambient = light_ambient * floorMat.material_ambient;
	floor_diffuse = light_diffuse * floorMat.material_diffuse;
	floor_specular = light_specular * floorMat.material_specular;

	// 2. Walls: Futuristic Blue-Grey Architectural Composite Panels (Clean high-contrast look)
	wallMat.material_ambient = vec4(0.14f, 0.16f, 0.20f, 1.0f);
	wallMat.material_diffuse = vec4(0.36f, 0.40f, 0.46f, 1.0f);
	wallMat.material_specular = vec4(0.30f, 0.35f, 0.42f, 1.0f);
	wallMat.material_shininess = 40.0f;
	wall_ambient = light_ambient * wallMat.material_ambient;
	wall_diffuse = light_diffuse * wallMat.material_diffuse;
	wall_specular = light_specular * wallMat.material_specular;

	// 3. Beams & Columns: Dark Graphite Structural Steel Frame
	beamMat.material_ambient = vec4(0.08f, 0.10f, 0.13f, 1.0f);
	beamMat.material_diffuse = vec4(0.18f, 0.20f, 0.25f, 1.0f);
	beamMat.material_specular = vec4(0.55f, 0.60f, 0.68f, 1.0f);
	beamMat.material_shininess = 75.0f;
	beam_ambient = light_ambient * beamMat.material_ambient;
	beam_diffuse = light_diffuse * beamMat.material_diffuse;
	beam_specular = light_specular * beamMat.material_specular;

	// 4. Cyan Light Conduits & Data Strips (Reserved for high-tech accent)
	cyanLightMat.material_ambient = vec4(0.15f, 0.70f, 0.90f, 1.0f);
	cyanLightMat.material_diffuse = vec4(0.30f, 0.85f, 1.00f, 1.0f);
	cyanLightMat.material_specular = vec4(0.90f, 1.00f, 1.00f, 1.0f);
	cyanLightMat.material_shininess = 90.0f;
	cyan_ambient = light_ambient * cyanLightMat.material_ambient;
	cyan_diffuse = light_diffuse * cyanLightMat.material_diffuse;
	cyan_specular = light_specular * cyanLightMat.material_specular;

	// 5. Warning Indicators & Hazard Stripes: Vibrant Amber/Orange
	warningMat.material_ambient = vec4(0.60f, 0.32f, 0.08f, 1.0f);
	warningMat.material_diffuse = vec4(0.95f, 0.52f, 0.12f, 1.0f);
	warningMat.material_specular = vec4(0.85f, 0.65f, 0.30f, 1.0f);
	warningMat.material_shininess = 60.0f;
	warn_ambient = light_ambient * warningMat.material_ambient;
	warn_diffuse = light_diffuse * warningMat.material_diffuse;
	warn_specular = light_specular * warningMat.material_specular;

	// 6. Door Frame: Heavy Reinforced Titanium Blast Portal
	doorFrameMat.material_ambient = vec4(0.10f, 0.12f, 0.15f, 1.0f);
	doorFrameMat.material_diffuse = vec4(0.25f, 0.28f, 0.34f, 1.0f);
	doorFrameMat.material_specular = vec4(0.65f, 0.70f, 0.75f, 1.0f);
	doorFrameMat.material_shininess = 70.0f;
	doorFrame_ambient = light_ambient * doorFrameMat.material_ambient;
	doorFrame_diffuse = light_diffuse * doorFrameMat.material_diffuse;
	doorFrame_specular = light_specular * doorFrameMat.material_specular;

	// 7. Door Blast Panels: Carbon-Steel Blast Sliders with Warning Chevron Accents
	doorPanelMat.material_ambient = vec4(0.08f, 0.10f, 0.13f, 1.0f);
	doorPanelMat.material_diffuse = vec4(0.20f, 0.23f, 0.28f, 1.0f);
	doorPanelMat.material_specular = vec4(0.40f, 0.45f, 0.50f, 1.0f);
	doorPanelMat.material_shininess = 40.0f;
	doorPanel_ambient = light_ambient * doorPanelMat.material_ambient;
	doorPanel_diffuse = light_diffuse * doorPanelMat.material_diffuse;
	doorPanel_specular = light_specular * doorPanelMat.material_specular;

	// 8. Cargo Crates: Industrial Steel Containers with Hazard Bands
	crateMat.material_ambient = vec4(0.14f, 0.16f, 0.20f, 1.0f);
	crateMat.material_diffuse = vec4(0.35f, 0.40f, 0.48f, 1.0f);
	crateMat.material_specular = vec4(0.60f, 0.68f, 0.75f, 1.0f);
	crateMat.material_shininess = 60.0f;
	crate_ambient = light_ambient * crateMat.material_ambient;
	crate_diffuse = light_diffuse * crateMat.material_diffuse;
	crate_specular = light_specular * crateMat.material_specular;

	// 9. Ceiling: Dark Architectural Decking with Overhead Troffer Luminaires
	ceilingMat.material_ambient = vec4(0.12f, 0.14f, 0.18f, 1.0f);
	ceilingMat.material_diffuse = vec4(0.30f, 0.34f, 0.38f, 1.0f);
	ceilingMat.material_specular = vec4(0.32f, 0.36f, 0.42f, 1.0f);
	ceilingMat.material_shininess = 35.0f;
	ceiling_ambient = light_ambient * ceilingMat.material_ambient;
	ceiling_diffuse = light_diffuse * ceilingMat.material_diffuse;
	ceiling_specular = light_specular * ceilingMat.material_specular;
}

void Room::addQuad(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3,
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

void Room::addBox(float minX, float minY, float minZ,
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

void Room::addCrate(float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ,
                    std::vector<vec4>& pts, std::vector<vec3>& norms,
                    std::vector<vec4>& beamPts, std::vector<vec3>& beamNorms,
                    std::vector<vec4>& warnPts, std::vector<vec3>& warnNorms) {
	crates.push_back({ minX, minY, minZ, maxX, maxY, maxZ });
	addBox(minX, minY, minZ, maxX, maxY, maxZ, pts, norms);

	const float r = 0.05f;
	// Corner trims
	addBox(minX - r, minY - r, minZ, maxX + r, maxY + r, minZ + 0.10f, beamPts, beamNorms);
	addBox(minX - r, minY - r, maxZ - 0.10f, maxX + r, maxY + r, maxZ, beamPts, beamNorms);
	addBox(minX - r, minY - r, minZ, minX + 0.08f, minY + 0.08f, maxZ, beamPts, beamNorms);
	addBox(maxX - 0.08f, minY - r, minZ, maxX + r, minY + 0.08f, maxZ, beamPts, beamNorms);
	addBox(minX - r, maxY - 0.08f, minZ, minX + 0.08f, maxY + r, maxZ, beamPts, beamNorms);
	addBox(maxX - 0.08f, maxY - 0.08f, minZ, maxX + r, maxY + r, maxZ, beamPts, beamNorms);

	// Amber warning stripe
	float midZ = (minZ + maxZ) * 0.5f;
	addBox(minX - 0.02f, minY - 0.02f, midZ - 0.06f, maxX + 0.02f, maxY + 0.02f, midZ + 0.06f, warnPts, warnNorms);
}

void Room::buildGeometry(GLuint program) {
	crates.clear();

	std::vector<vec4> floorPts, wallPts, beamPts, cyanLightPts, warnPts, doorFramePts, doorPanelPts, cratePts, ceilingPts;
	std::vector<vec3> floorNrms, wallNrms, beamNrms, cyanLightNrms, warnNrms, doorFrameNrms, doorPanelNrms, crateNrms, ceilingNrms;

	// -------------------------------------------------------------
	// 1. FLOOR: Silver-Metallic Research Decking with Expansion Grooves
	// -------------------------------------------------------------
	addBox(MIN_X, MIN_Y, -0.3f, MAX_X, MAX_Y, 0.0f, floorPts, floorNrms);

	const float tileW = 2.70f;
	const float tileL = 3.65f;
	const float gap = 0.10f;
	for (float x = MIN_X + 0.35f; x <= MAX_X - tileW; x += (tileW + gap)) {
		for (float y = MIN_Y + 0.35f; y <= MAX_Y - tileL; y += (tileL + gap)) {
			addBox(x, y, 0.0f, x + tileW, y + tileL, 0.05f, floorPts, floorNrms);
		}
	}
	// Floor Baseboards
	addBox(MIN_X, MIN_Y, 0.0f, MIN_X + 0.30f, MAX_Y, 0.15f, beamPts, beamNrms);
	addBox(MAX_X - 0.30f, MIN_Y, 0.0f, MAX_X, MAX_Y, 0.15f, beamPts, beamNrms);
	addBox(MIN_X, MIN_Y, 0.0f, MAX_X, MIN_Y + 0.30f, 0.15f, beamPts, beamNrms);

	// Cyan Floor Guide Strips
	addBox(-0.15f, MIN_Y + 1.0f, 0.051f, 0.15f, MAX_Y - 2.0f, 0.065f, cyanLightPts, cyanLightNrms);
	addBox(-5.5f, MIN_Y + 1.0f, 0.051f, -5.3f, MAX_Y - 2.0f, 0.065f, cyanLightPts, cyanLightNrms);
	addBox( 5.3f, MIN_Y + 1.0f, 0.051f,  5.5f, MAX_Y - 2.0f, 0.065f, cyanLightPts, cyanLightNrms);

	// -------------------------------------------------------------
	// 2. WALLS: Off-White Composite Panels
	// -------------------------------------------------------------
	// Outer bulkheads
	addBox(MIN_X - 0.3f, MIN_Y, 0.0f, MIN_X, MAX_Y, MAX_Z, wallPts, wallNrms);
	addBox(MAX_X, MIN_Y, 0.0f, MAX_X + 0.3f, MAX_Y, MAX_Z, wallPts, wallNrms);
	addBox(MIN_X, MIN_Y - 0.3f, 0.0f, MAX_X, MIN_Y, MAX_Z, wallPts, wallNrms);

	// Front Exit Door Bulkhead (Cutout: X in [-2.4, 2.4], Z in [0, 4.6])
	addBox(MIN_X, MAX_Y, 0.0f, -2.4f, MAX_Y + 0.3f, MAX_Z, wallPts, wallNrms);
	addBox(2.4f, MAX_Y, 0.0f, MAX_X, MAX_Y + 0.3f, MAX_Z, wallPts, wallNrms);
	addBox(-2.4f, MAX_Y, 4.6f, 2.4f, MAX_Y + 0.3f, MAX_Z, wallPts, wallNrms);

	// Recessed Ventilation Louvers along Walls
	for (float y = MIN_Y + 3.0f; y <= MAX_Y - 5.0f; y += 7.5f) {
		addBox(MIN_X, y, 0.4f, MIN_X + 0.08f, y + 2.4f, 1.4f, wallPts, wallNrms);
		for (float vz = 0.55f; vz <= 1.25f; vz += 0.22f) {
			addBox(MIN_X + 0.02f, y + 0.15f, vz, MIN_X + 0.07f, y + 2.25f, vz + 0.08f, beamPts, beamNrms);
		}
		addBox(MAX_X - 0.08f, y, 0.4f, MAX_X, y + 2.4f, 1.4f, wallPts, wallNrms);
		for (float vz = 0.55f; vz <= 1.25f; vz += 0.22f) {
			addBox(MAX_X - 0.07f, y + 0.15f, vz, MAX_X - 0.02f, y + 2.25f, vz + 0.08f, beamPts, beamNrms);
		}
	}

	// -------------------------------------------------------------
	// 3. GRAPHITE BEAMS, ARCHITECTURAL COLUMNS & CONSOLES
	// -------------------------------------------------------------
	// 4 Heavy Structural Corner Columns
	addBox(MIN_X, MIN_Y, 0.0f, MIN_X + 0.65f, MIN_Y + 0.65f, MAX_Z, beamPts, beamNrms);
	addBox(MAX_X - 0.65f, MIN_Y, 0.0f, MAX_X, MIN_Y + 0.65f, MAX_Z, beamPts, beamNrms);
	addBox(MIN_X, MAX_Y - 0.65f, 0.0f, MIN_X + 0.65f, MAX_Y, MAX_Z, beamPts, beamNrms);
	addBox(MAX_X - 0.65f, MAX_Y - 0.65f, 0.0f, MAX_X, MAX_Y, MAX_Z, beamPts, beamNrms);

	// Intermediate Pillars & Transverse Ceiling Girders
	const float pillarY[] = { -12.0f, -4.0f, 4.0f, 12.0f };
	for (float py : pillarY) {
		addBox(MIN_X, py - 0.35f, 0.0f, MIN_X + 0.45f, py + 0.35f, MAX_Z, beamPts, beamNrms);
		addBox(MAX_X - 0.45f, py - 0.35f, 0.0f, MAX_X, py + 0.35f, MAX_Z, beamPts, beamNrms);
		addBox(MIN_X + 0.45f, py - 0.22f, MAX_Z - 0.38f, MAX_X - 0.45f, py + 0.22f, MAX_Z, beamPts, beamNrms);
		// Horizontal Wall Stringer Reinforcements
		addBox(MIN_X, py - 3.75f, 3.6f, MIN_X + 0.12f, py + 3.75f, 3.8f, beamPts, beamNrms);
		addBox(MAX_X - 0.12f, py - 3.75f, 3.6f, MAX_X, py + 3.75f, 3.8f, beamPts, beamNrms);
	}

	// Wall-Mounted Research Consoles (Screens with cyan interfaces)
	const float consoleY[] = { -8.0f, 0.0f, 8.0f };
	for (float cy : consoleY) {
		// Left Wall Console
		addBox(MIN_X, cy - 0.8f, 1.2f, MIN_X + 0.4f, cy + 0.8f, 2.1f, beamPts, beamNrms);
		addBox(MIN_X + 0.405f, cy - 0.6f, 1.4f, MIN_X + 0.42f, cy + 0.6f, 1.95f, cyanLightPts, cyanLightNrms);
		// Right Wall Console
		addBox(MAX_X - 0.4f, cy - 0.8f, 1.2f, MAX_X, cy + 0.8f, 2.1f, beamPts, beamNrms);
		addBox(MAX_X - 0.42f, cy - 0.6f, 1.4f, MAX_X - 0.405f, cy + 0.6f, 1.95f, cyanLightPts, cyanLightNrms);
	}

	// -------------------------------------------------------------
	// 4. MECHANICAL EXIT PORTAL & KEYCARD READER
	// -------------------------------------------------------------
	// Outer Heavy Door Frame
	addBox(-2.8f, MAX_Y - 0.35f, 0.0f, -2.4f, MAX_Y + 0.35f, 4.9f, doorFramePts, doorFrameNrms);
	addBox( 2.4f, MAX_Y - 0.35f, 0.0f,  2.8f, MAX_Y + 0.35f, 4.9f, doorFramePts, doorFrameNrms);
	addBox(-2.8f, MAX_Y - 0.35f, 4.6f,  2.8f, MAX_Y + 0.35f, 5.0f, doorFramePts, doorFrameNrms);

	// Luminous Cyan Door Archway Trim
	addBox(-2.45f, MAX_Y - 0.28f, 0.05f, -2.35f, MAX_Y + 0.28f, 4.65f, cyanLightPts, cyanLightNrms);
	addBox( 2.35f, MAX_Y - 0.28f, 0.05f,  2.45f, MAX_Y + 0.28f, 4.65f, cyanLightPts, cyanLightNrms);
	addBox(-2.45f, MAX_Y - 0.28f, 4.55f,  2.45f, MAX_Y + 0.28f, 4.65f, cyanLightPts, cyanLightNrms);

	// Amber Hazard Striping at Threshold
	addBox(-2.4f, MAX_Y - 0.6f, 0.051f, 2.4f, MAX_Y - 0.35f, 0.065f, warnPts, warnNrms);

	// Door Control Panel & Keycard Reader Post (Right side of door)
	addBox(2.9f, MAX_Y - 0.5f, 1.3f, 3.4f, MAX_Y - 0.2f, 2.1f, beamPts, beamNrms);
	addBox(2.95f, MAX_Y - 0.52f, 1.55f, 3.35f, MAX_Y - 0.48f, 1.95f, cyanLightPts, cyanLightNrms); // Reader screen

	// Sliding Blast Door Panels
	addBox(-2.38f, MAX_Y - 0.06f, 0.0f, -0.04f, MAX_Y + 0.06f, 4.55f, doorPanelPts, doorPanelNrms);
	addBox( 0.04f, MAX_Y - 0.06f, 0.0f,  2.38f, MAX_Y + 0.06f, 4.55f, doorPanelPts, doorPanelNrms);

	// -------------------------------------------------------------
	// 5. CEILING & WARM WHITE OVERHEAD LIGHT FIXTURES
	// -------------------------------------------------------------
	addBox(MIN_X, MIN_Y, MAX_Z, MAX_X, MAX_Y, MAX_Z + 0.3f, ceilingPts, ceilingNrms);

	// Long Warm White Ceiling Troffers
	for (float y = MIN_Y + 2.0f; y <= MAX_Y - 4.0f; y += 8.0f) {
		addBox(-1.2f, y, MAX_Z - 0.12f, 1.2f, y + 4.5f, MAX_Z, beamPts, beamNrms);
		addBox(-0.9f, y + 0.2f, MAX_Z - 0.13f, 0.9f, y + 4.3f, MAX_Z - 0.09f, cyanLightPts, cyanLightNrms);
	}

	// Calculate buffer offsets and counts
	floorOffset = 0;
	floorCount = (GLsizei)floorPts.size();

	wallOffset = floorOffset + floorCount;
	wallCount = (GLsizei)wallPts.size();

	beamOffset = wallOffset + wallCount;
	beamCount = (GLsizei)beamPts.size();

	cyanLightOffset = beamOffset + beamCount;
	cyanLightCount = (GLsizei)cyanLightPts.size();

	warningOffset = cyanLightOffset + cyanLightCount;
	warningCount = (GLsizei)warnPts.size();

	doorFrameOffset = warningOffset + warningCount;
	doorFrameCount = (GLsizei)doorFramePts.size();

	doorPanelOffset = doorFrameOffset + doorFrameCount;
	doorPanelCount = (GLsizei)doorPanelPts.size();

	crateOffset = doorPanelOffset + doorPanelCount;
	crateCount = (GLsizei)cratePts.size();

	ceilingOffset = crateOffset + crateCount;
	ceilingCount = (GLsizei)ceilingPts.size();

	GLsizei totalVerts = ceilingOffset + ceilingCount;

	std::vector<vec4> allPts;
	std::vector<vec3> allNrms;
	allPts.reserve(totalVerts);
	allNrms.reserve(totalVerts);

	auto append = [&](const std::vector<vec4>& p, const std::vector<vec3>& n) {
		allPts.insert(allPts.end(), p.begin(), p.end());
		allNrms.insert(allNrms.end(), n.begin(), n.end());
	};

	append(floorPts, floorNrms);
	append(wallPts, wallNrms);
	append(beamPts, beamNrms);
	append(cyanLightPts, cyanLightNrms);
	append(warnPts, warnNrms);
	append(doorFramePts, doorFrameNrms);
	append(doorPanelPts, doorPanelNrms);
	append(cratePts, crateNrms);
	append(ceilingPts, ceilingNrms);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	GLsizeiptr posBytes = totalVerts * sizeof(vec4);
	GLsizeiptr normBytes = totalVerts * sizeof(vec3);
	glBufferData(GL_ARRAY_BUFFER, posBytes + normBytes, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, posBytes, allPts.data());
	glBufferSubData(GL_ARRAY_BUFFER, posBytes, normBytes, allNrms.data());

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)posBytes);

	glBindVertexArray(0);
}

void Room::draw(mat4 view, GLuint program, GLuint modelView_ptr) {
	if (vao == 0) return;

	glBindVertexArray(vao);
	glUniformMatrix4fv(modelView_ptr, 1, GL_TRUE, view);

	auto setMat = [&](const vec4& amb, const vec4& diff, const vec4& spec, float shin) {
		glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, amb);
		glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diff);
		glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, spec);
		glUniform1f(glGetUniformLocation(program, "Shininess"), shin);
	};

	// 1. Floor
	setMat(floor_ambient, floor_diffuse, floor_specular, floorMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, floorOffset, floorCount);

	// 2. Walls
	setMat(wall_ambient, wall_diffuse, wall_specular, wallMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, wallOffset, wallCount);

	// 3. Beams, Columns & Consoles
	setMat(beam_ambient, beam_diffuse, beam_specular, beamMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, beamOffset, beamCount);

	// 4. Cyan Light Conduits
	setMat(cyan_ambient, cyan_diffuse, cyan_specular, cyanLightMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, cyanLightOffset, cyanLightCount);

	// 5. Warning Indicators & Hazard Stripes
	setMat(warn_ambient, warn_diffuse, warn_specular, warningMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, warningOffset, warningCount);

	// 6. Door Frame (Subtle cyan pulse highlight marking interactive sector exit portal)
	float doorPulse = 0.5f + 0.5f * std::sin(doorPulseTimer * 2.8f);
	vec4 doorCyanGlow(0.05f * doorPulse, 0.20f * doorPulse, 0.26f * doorPulse, 0.0f);
	setMat(doorFrame_ambient + doorCyanGlow * 0.4f,
	       doorFrame_diffuse + doorCyanGlow * 0.7f,
	       doorFrame_specular + doorCyanGlow * 1.0f,
	       doorFrameMat.material_shininess + doorPulse * 20.0f);
	glDrawArrays(GL_TRIANGLES, doorFrameOffset, doorFrameCount);

	// 7. Door Blast Panels
	setMat(doorPanel_ambient, doorPanel_diffuse, doorPanel_specular, doorPanelMat.material_shininess);
	glDrawArrays(GL_TRIANGLES, doorPanelOffset, doorPanelCount);

	// 8. Ceiling (Toggleable)
	if (showCeiling) {
		setMat(ceiling_ambient, ceiling_diffuse, ceiling_specular, ceilingMat.material_shininess);
		glDrawArrays(GL_TRIANGLES, ceilingOffset, ceilingCount);
	}

	glBindVertexArray(0);
}
