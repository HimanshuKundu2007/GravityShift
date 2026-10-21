#include "Camera.h"
#include "Room.h"

Camera::Camera() {
	this->focal = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	this->position = vec4(0.0f, -6.0f, 3.0f, 1.0f);
	this->up = vec4(0.0f, 0.0f, 1.0f, 0.0f);
	this->view = LookAt(position, focal, up);
	this->currMode = VIEW_MODE::TP;
	this->prevMode = VIEW_MODE::TP;
	this->actor = nullptr;
}

void Camera::attach(Player* player) {
	this->actor = player;
}

/**
 * @brief Updates camera Eye, At, and Up vectors and synthesizes the View Matrix.
 * 
 * In Third-Person mode:
 * - Up vector dynamically matches the player's surface normal (getUpVector())
 *   so the camera smoothly re-orients when walking on walls or ceilings.
 * - Eye position is positioned behind the astronaut along the corridor (-Y)
 *   and elevated along the current surface normal.
 * - Margin clamping prevents the camera from clipping outside room bulkheads.
 */
void Camera::update() {
	if (!actor) return;

	// Surface normal vector aligned with current gravity orientation
	vec3 gravUp = actor->getUpVector();

	switch (this->currMode) {
	case VIEW_MODE::TP: {
		// Align camera up-vector with active surface gravity normal
		this->up = vec4(gravUp.x, gravUp.y, gravUp.z, 0.0f);

		const float distBehind = 5.8f;
		const float heightAbove = 1.8f;

		vec4 idealPos = actor->position
			- vec4(0.0f, distBehind, 0.0f, 0.0f)
			+ vec4(gravUp.x * heightAbove, gravUp.y * heightAbove, gravUp.z * heightAbove, 0.0f);

		// Clamp camera Eye inside room bulkheads to prevent wall clipping
		const float marginX = 0.70f;
		const float marginY = 0.70f;
		const float marginZ = 0.50f;

		if (idealPos.x < Room::MIN_X + marginX) idealPos.x = Room::MIN_X + marginX;
		if (idealPos.x > Room::MAX_X - marginX) idealPos.x = Room::MAX_X - marginX;
		if (idealPos.y < Room::MIN_Y + marginY) idealPos.y = Room::MIN_Y + marginY;
		if (idealPos.y > Room::MAX_Y - marginY) idealPos.y = Room::MAX_Y - marginY;
		if (idealPos.z < Room::MIN_Z + marginZ) idealPos.z = Room::MIN_Z + marginZ;
		if (idealPos.z > Room::MAX_Z - marginZ) idealPos.z = Room::MAX_Z - marginZ;

		this->position = idealPos;

		// Focus on astronaut's upper torso and slightly ahead along forward travel axis
		this->focal = actor->position
			+ vec4(0.0f, 0.8f, 0.0f, 0.0f)
			+ vec4(gravUp.x * 0.25f, gravUp.y * 0.25f, gravUp.z * 0.25f, 0.0f);
		break;
	}
	case VIEW_MODE::FP: {
		this->up = vec4(gravUp.x, gravUp.y, gravUp.z, 0.0f);
		this->position = actor->position + vec4(gravUp.x * 0.65f, 0.20f, gravUp.z * 0.65f, 0.0f);
		this->focal = actor->position + vec4(gravUp.x * 0.65f, 8.0f, gravUp.z * 0.65f, 0.0f);
		break;
	}
	case VIEW_MODE::TOP: {
		this->up = vec4(0.0f, 1.0f, 0.0f, 0.0f); // Forward corridor axis (+Y) is UP
		this->position = vec4(0.01f, 1.0f, 26.0f, 1.0f);
		this->focal = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		break;
	}
	}

	// Compute View Matrix via LookAt(eye, at, up)
	this->view = LookAt(position, focal, up);
}
