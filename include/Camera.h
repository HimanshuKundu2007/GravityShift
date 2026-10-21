#pragma once
#include "Angel/Angel.h"
#include "Player.h"

/**
 * @brief Dynamic 3D Camera system for Gravity Shift.
 * 
 * Supports Third-Person (TP), First-Person (FP), and Top-Down (TOP) orthographic-like modes.
 * Calculates view transformation matrix via LookAt(eye, at, up) with up-vector alignment
 * to match the player's active surface normal and gravity orientation.
 */
class Camera {
public:
	enum VIEW_MODE {
		FP = 0, // First Person View
		TP = 1, // Third Person View (Default)
		TOP = 2 // Top-Down Tactical Overview
	};

	Camera();
	void attach(Player* player);
	void update();

	mat4 view;          ///< Computed View transformation matrix (World -> Camera coordinates)
	VIEW_MODE currMode; ///< Current active camera view mode
	VIEW_MODE prevMode; ///< Previously active camera view mode

private:
	vec4 position;      ///< Camera Eye position in world space
	vec4 focal;         ///< Camera LookAt Target point in world space
	vec4 up;            ///< Camera Up direction vector
	Player* actor;      ///< Pointer to tracked player character
};