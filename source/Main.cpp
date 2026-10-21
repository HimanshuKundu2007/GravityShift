#include "Angel/Angel.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "World.h"
#include "Camera.h"
#include <GL/freeglut_ext.h>

#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))

World* world = nullptr;
GLuint mainProgram = 0;

// Input key tracking
bool keys[256] = { false };
bool specialKeys[256] = { false };

std::string readFile(const char *filePath) {
	std::string content;
	std::ifstream fileStream(filePath, std::ios::in);
	if (!fileStream.is_open()) {
		std::cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
		return "";
	}
	std::string line = "";
	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		content.append(line + "\n");
	}
	fileStream.close();
	return content;
}

GLuint LoadShader(const char *vertex_path, const char *fragment_path) {
	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertShaderStr = readFile(vertex_path);
	std::string fragShaderStr = readFile(fragment_path);
	const char *vertShaderSrc = vertShaderStr.c_str();
	const char *fragShaderSrc = fragShaderStr.c_str();

	GLint result = GL_FALSE;
	int logLength;

	glShaderSource(vertShader, 1, &vertShaderSrc, NULL);
	glCompileShader(vertShader);
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(vertShader, logLength, NULL, &vertShaderError[0]);

	glShaderSource(fragShader, 1, &fragShaderSrc, NULL);
	glCompileShader(fragShader);
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<GLchar> fragShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(fragShader, logLength, NULL, &fragShaderError[0]);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
	std::vector<char> programError((logLength > 1) ? logLength : 1);
	glGetProgramInfoLog(program, logLength, NULL, &programError[0]);

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return program;
}

void renderHUD() {
	int w = glutGet(GLUT_WINDOW_WIDTH);
	int h = glutGet(GLUT_WINDOW_HEIGHT);
	if (w <= 0 || h <= 0) return;

	glUseProgram(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	auto drawString = [](float x, float y, const std::string& str, void* font = GLUT_BITMAP_HELVETICA_12) {
		glRasterPos2f(x, y);
		for (char c : str) {
			glutBitmapCharacter(font, c);
		}
	};

	auto drawPanel = [](float x, float y, float pw, float ph, const vec4& bg, const vec4& border, float lineWidth = 1.6f) {
		glColor4fv(bg);
		glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + pw, y);
			glVertex2f(x + pw, y - ph);
			glVertex2f(x, y - ph);
		glEnd();

		glColor4fv(border);
		glLineWidth(lineWidth);
		glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x + pw, y);
			glVertex2f(x + pw, y - ph);
			glVertex2f(x, y - ph);
		glEnd();
	};

	auto drawBadge = [](float x, float y, float bw, float bh, const vec4& bg, const vec4& border) {
		glColor4fv(bg);
		glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + bw, y);
			glVertex2f(x + bw, y - bh);
			glVertex2f(x, y - bh);
		glEnd();

		glColor4fv(border);
		glLineWidth(1.2f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(x, y);
			glVertex2f(x + bw, y);
			glVertex2f(x + bw, y - bh);
			glVertex2f(x, y - bh);
		glEnd();
	};

	const float padX = 14.0f;
	const float topY = h - 14.0f;
	const float cardH = 114.0f;

	// Responsive panel widths: avoid overlaps on lower resolutions
	float cardW = 500.0f;
	float gravCardW = 280.0f;
	if (w < (cardW + gravCardW + padX * 3.0f)) {
		float avail = w - padX * 3.0f;
		if (avail > 320.0f) {
			cardW = avail * 0.62f;
			gravCardW = avail * 0.38f;
		}
	}
	const float gravX = w - gravCardW - padX;

	// -------------------------------------------------------------
	// 1. TOP-LEFT MISSION HUD CARD
	// -------------------------------------------------------------
	drawPanel(padX, topY, cardW, cardH, vec4(0.015f, 0.035f, 0.07f, 0.90f), vec4(0.0f, 0.85f, 1.0f, 0.85f), 1.6f);

	// Header accent tab
	float tabW = (cardW * 0.44f < 220.0f) ? (cardW * 0.44f) : 220.0f;
	glColor4f(0.0f, 0.95f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
		glVertex2f(padX, topY);
		glVertex2f(padX + tabW, topY);
		glVertex2f(padX + tabW - 15.0f, topY - 22.0f);
		glVertex2f(padX, topY - 22.0f);
	glEnd();

	glColor3f(0.02f, 0.08f, 0.15f);
	drawString(padX + 12.0f, topY - 16.0f, "GRAVITY SHIFT // SECTOR");

	if (world && world->levelManager) {
		glColor3f(1.0f, 0.88f, 0.20f);
		drawString(padX + tabW + 10.0f, topY - 16.0f, world->levelManager->getLevelName());

		glColor3f(0.80f, 0.94f, 1.0f);
		drawString(padX + 12.0f, topY - 40.0f, world->levelManager->getObjectiveText());

		// Live Score, Timer and Collision Metrics
		char scoreBuf[128];
		sprintf_s(scoreBuf, "SCORE: %04d pts   |   TIME: %04.1fs   |   COLLISIONS: %d",
		          world->levelManager->getLiveLevelScore(),
		          world->levelManager->getElapsedTime(),
		          world->levelManager->getCurrentCollisions());
		glColor3f(0.20f, 1.0f, 0.85f);
		drawString(padX + 12.0f, topY - 64.0f, scoreBuf);

		// Performance Bonus Badges
		float badgeY = topY - 76.0f;
		if (world->levelManager->getCurrentCollisions() == 0) {
			drawBadge(padX + 12.0f, badgeY, 136.0f, 20.0f, vec4(0.02f, 0.16f, 0.08f, 0.90f), vec4(0.20f, 1.0f, 0.50f, 0.85f));
			glColor3f(0.35f, 1.0f, 0.55f);
			drawString(padX + 18.0f, badgeY - 14.0f, "[ CLEAN RUN +100 ]");
		} else {
			drawBadge(padX + 12.0f, badgeY, 136.0f, 20.0f, vec4(0.12f, 0.05f, 0.05f, 0.80f), vec4(0.60f, 0.40f, 0.40f, 0.70f));
			glColor3f(0.85f, 0.65f, 0.65f);
			char colBuf[32]; sprintf_s(colBuf, "[ IMPACTS: %d ]", world->levelManager->getCurrentCollisions());
			drawString(padX + 18.0f, badgeY - 14.0f, colBuf);
		}

		if (!world->levelManager->isCurrentRestarted()) {
			drawBadge(padX + 154.0f, badgeY, 136.0f, 20.0f, vec4(0.02f, 0.16f, 0.08f, 0.90f), vec4(0.20f, 1.0f, 0.50f, 0.85f));
			glColor3f(0.35f, 1.0f, 0.55f);
			drawString(padX + 160.0f, badgeY - 14.0f, "[ NO-RETRY +100 ]");
		} else {
			drawBadge(padX + 154.0f, badgeY, 136.0f, 20.0f, vec4(0.14f, 0.10f, 0.03f, 0.80f), vec4(0.70f, 0.60f, 0.30f, 0.70f));
			glColor3f(0.90f, 0.75f, 0.40f);
			drawString(padX + 160.0f, badgeY - 14.0f, "[ RETRIED: NO BONUS ]");
		}

		// Oxygen Life Support Countdown Bar (Level 5)
		if (world->levelManager->isOxygenActive()) {
			float oxy = world->levelManager->getOxygenTime();
			float maxOxy = 65.0f;
			float ratio = oxy / maxOxy;
			if (ratio < 0.0f) ratio = 0.0f;
			if (ratio > 1.0f) ratio = 1.0f;

			float oxyBarX = padX + 298.0f;
			float oxyBarW = cardW - 310.0f;
			if (oxyBarW > 60.0f) {
				drawBadge(oxyBarX, badgeY, oxyBarW, 20.0f, vec4(0.15f, 0.02f, 0.02f, 0.90f), vec4(1.0f, 0.30f, 0.30f, 0.85f));

				vec4 barColor = (ratio > 0.40f) ? vec4(0.10f, 0.90f, 0.35f, 0.95f) :
				                ((ratio > 0.20f) ? vec4(1.0f, 0.80f, 0.10f, 0.95f) : vec4(1.0f, 0.20f, 0.20f, 0.95f));
				glColor4fv(barColor);
				glBegin(GL_QUADS);
					glVertex2f(oxyBarX + 1.0f, badgeY - 1.0f);
					glVertex2f(oxyBarX + 1.0f + ratio * (oxyBarW - 2.0f), badgeY - 1.0f);
					glVertex2f(oxyBarX + 1.0f + ratio * (oxyBarW - 2.0f), badgeY - 19.0f);
					glVertex2f(oxyBarX + 1.0f, badgeY - 19.0f);
				glEnd();

				char oxyBuf[32]; sprintf_s(oxyBuf, "O2: %.1fs", oxy);
				glColor3f(1.0f, 1.0f, 1.0f);
				drawString(oxyBarX + 8.0f, badgeY - 14.0f, oxyBuf);
			}
		}
	}

	// -------------------------------------------------------------
	// 2. TOP-RIGHT GRAVITY ORIENTATION & INVENTORY CARD
	// -------------------------------------------------------------
	if (world && world->player) {
		float gravFlash = world->player->gravityFlashTimer;
		vec4 gravBorder = (gravFlash > 0.0f) ? vec4(0.35f, 1.0f, 1.0f, 1.0f) : vec4(0.0f, 0.85f, 1.0f, 0.85f);
		float lineWidth = (gravFlash > 0.0f) ? 2.5f : 1.6f;
		vec4 gravBg = (gravFlash > 0.0f) ? vec4(0.03f, 0.08f, 0.16f, 0.95f) : vec4(0.015f, 0.035f, 0.07f, 0.90f);

		drawPanel(gravX, topY, gravCardW, cardH, gravBg, gravBorder, lineWidth);

		// Line 1: Gravity Direction Status
		if (gravFlash > 0.0f) {
			char gravBuf[64];
			sprintf_s(gravBuf, ">> SHIFTING: %s %s <<", world->player->getGravityName(), world->player->getGravityArrow());
			glColor3f(0.30f, 1.0f, 1.0f);
			drawString(gravX + 14.0f, topY - 24.0f, gravBuf);
		} else {
			char gravBuf[64];
			sprintf_s(gravBuf, "GRAVITY: %s %s", world->player->getGravityName(), world->player->getGravityArrow());
			glColor3f(1.0f, 0.88f, 0.20f);
			drawString(gravX + 14.0f, topY - 24.0f, gravBuf);
		}

		// Line 2: Grounded vs Airborne Status Badge
		float stBadgeY = topY - 36.0f;
		if (world->player->isGrounded) {
			drawBadge(gravX + 14.0f, stBadgeY, gravCardW - 28.0f, 22.0f, vec4(0.02f, 0.16f, 0.08f, 0.95f), vec4(0.20f, 1.0f, 0.50f, 0.90f));
			glColor3f(0.25f, 1.0f, 0.55f);
			drawString(gravX + 22.0f, stBadgeY - 15.0f, "[ GROUNDED ] SURFACE CONTACT LOCKED");
		} else {
			float airPulse = 0.85f + 0.15f * std::sin(world->levelManager ? world->levelManager->getElapsedTime() * 6.0f : 0.0f);
			drawBadge(gravX + 14.0f, stBadgeY, gravCardW - 28.0f, 22.0f, vec4(0.18f * airPulse, 0.09f * airPulse, 0.02f, 0.95f), vec4(1.0f, 0.70f * airPulse, 0.20f, 0.95f));
			glColor3f(1.0f, 0.75f, 0.25f);
			drawString(gravX + 22.0f, stBadgeY - 15.0f, "[ AIRBORNE ] ZERO-G / FREEFALL DRIFT");
		}

		// Line 3: Keycard Clearance Status Badge
		float keyBadgeY = topY - 62.0f;
		if (world->levelManager && world->levelManager->isKeycardNeeded()) {
			if (world->levelManager->isKeycardCollected()) {
				drawBadge(gravX + 14.0f, keyBadgeY, gravCardW - 28.0f, 20.0f, vec4(0.02f, 0.14f, 0.16f, 0.90f), vec4(0.20f, 0.90f, 1.0f, 0.85f));
				glColor3f(0.30f, 1.0f, 0.90f);
				drawString(gravX + 22.0f, keyBadgeY - 14.0f, "[ KEYCARD ACQUIRED // UNLOCKED ]");
			} else {
				drawBadge(gravX + 14.0f, keyBadgeY, gravCardW - 28.0f, 20.0f, vec4(0.18f, 0.04f, 0.04f, 0.90f), vec4(1.0f, 0.30f, 0.25f, 0.85f));
				glColor3f(1.0f, 0.40f, 0.35f);
				drawString(gravX + 22.0f, keyBadgeY - 14.0f, "[ KEYCARD REQUIRED // DOOR LOCKED ]");
			}
		} else {
			drawBadge(gravX + 14.0f, keyBadgeY, gravCardW - 28.0f, 20.0f, vec4(0.03f, 0.06f, 0.10f, 0.80f), vec4(0.30f, 0.50f, 0.70f, 0.65f));
			glColor3f(0.65f, 0.82f, 0.95f);
			drawString(gravX + 22.0f, keyBadgeY - 14.0f, "[ CLEARANCE: OPEN PASSAGE ]");
		}

		// Line 4: Essential Controls Prompt
		glColor3f(0.60f, 0.85f, 1.0f);
		drawString(gravX + 14.0f, topY - 96.0f, "SHIFT GRAVITY: [G]   |   JUMP: [SPACE]");
	}

	// -------------------------------------------------------------
	// 3. START / LEVEL INTRODUCTION BANNER (Auto-dismisses or on key)
	// -------------------------------------------------------------
	if (world && world->levelManager && world->levelManager->getState() == LevelManager::GameState::PLAYING) {
		float introTimer = world->levelManager->getLevelIntroTimer();
		if (introTimer > 0.0f) {
			float introAlpha = (introTimer < 0.6f) ? (introTimer / 0.6f) : 1.0f;
			float cx = w * 0.5f;
			float cy = h * 0.5f + 45.0f;
			const float introW = std::min(600.0f, w - 40.0f);
			const float introH = 138.0f;

			drawPanel(cx - introW * 0.5f, cy + introH * 0.5f, introW, introH,
			          vec4(0.015f, 0.04f, 0.08f, 0.94f * introAlpha),
			          vec4(0.0f, 0.88f, 1.0f, 0.90f * introAlpha), 2.2f);

			// Decorative top banner line
			glColor4f(0.0f, 0.85f, 1.0f, 0.85f * introAlpha);
			glBegin(GL_LINES);
				glVertex2f(cx - introW * 0.5f + 16.0f, cy + introH * 0.5f - 30.0f);
				glVertex2f(cx + introW * 0.5f - 16.0f, cy + introH * 0.5f - 30.0f);
			glEnd();

			std::string lvlTitle = world->levelManager->getLevelName();
			glColor4f(1.0f, 0.88f, 0.20f, introAlpha);
			drawString(cx - (float)(lvlTitle.length() * 4.2f), cy + 46.0f, lvlTitle, GLUT_BITMAP_HELVETICA_18);

			std::string objStr = world->levelManager->getObjectiveText();
			glColor4f(0.80f, 0.94f, 1.0f, introAlpha);
			drawString(cx - (float)(objStr.length() * 3.6f), cy + 18.0f, objStr);

			std::string ctrlStr = "CONTROLS:  [W/S/A/D] Walk   [SPACE] Jump   [G] Shift Gravity   [E] Interact   [R] Restart";
			glColor4f(0.35f, 1.0f, 0.70f, introAlpha);
			drawString(cx - (float)(ctrlStr.length() * 3.6f), cy - 14.0f, ctrlStr);

			std::string promptStr = "COMMENCE MISSION // ENGAGE CONTROLS (AUTO-DISMISSES)";
			glColor4f(1.0f, 0.90f, 0.35f, 0.85f * introAlpha);
			drawString(cx - (float)(promptStr.length() * 3.6f), cy - 42.0f, promptStr);
		}
	}

	// -------------------------------------------------------------
	// 4. CENTER DYNAMIC INTERACTION PROMPT CARD
	// -------------------------------------------------------------
	if (world && world->levelManager && world->player) {
		std::string prompt = world->levelManager->getInteractionPrompt(world->player->position);
		if (!prompt.empty()) {
			float cx = w * 0.5f;
			float cy = 76.0f;
			const float pBoxW = std::min(520.0f, w - 60.0f);
			const float pBoxH = 38.0f;

			bool isLocked = (prompt.find("LOCKED") != std::string::npos || prompt.find("BLOCKED") != std::string::npos);
			bool isExit = (prompt.find("EXIT") != std::string::npos);

			vec4 bg = isLocked ? vec4(0.18f, 0.03f, 0.03f, 0.95f) :
			          (isExit ? vec4(0.02f, 0.14f, 0.07f, 0.95f) : vec4(0.01f, 0.05f, 0.10f, 0.95f));
			vec4 border = isLocked ? vec4(1.0f, 0.25f, 0.20f, 0.95f) :
			              (isExit ? vec4(0.20f, 1.0f, 0.50f, 0.95f) : vec4(0.0f, 0.90f, 1.0f, 0.95f));

			drawPanel(cx - pBoxW * 0.5f, cy + pBoxH * 0.5f, pBoxW, pBoxH, bg, border, 2.2f);

			if (isLocked) glColor3f(1.0f, 0.35f, 0.30f);
			else if (isExit) glColor3f(0.30f, 1.0f, 0.60f);
			else glColor3f(1.0f, 0.92f, 0.30f);

			drawString(cx - (float)(prompt.length() * 4.2f), cy - 4.0f, prompt);
		}
	}

	// -------------------------------------------------------------
	// 5. MODAL COMPLETION & ALERT SCREENS
	// -------------------------------------------------------------
	if (world && world->levelManager) {
		LevelManager::GameState st = world->levelManager->getState();

		// A. LEVEL COMPLETE MODAL
		if (st == LevelManager::GameState::LEVEL_COMPLETE) {
			float cx = w * 0.5f;
			float cy = h * 0.5f;
			const float baseBoxW = 540.0f;
			const float baseBoxH = 290.0f;

			// Delta-time animated entrance transition (0.35s quadratic ease-out)
			float animTimer = world->levelManager->getLevelCompleteAnimTimer();
			float tNorm = (animTimer < 0.35f) ? (animTimer / 0.35f) : 1.0f;
			float ease = 1.0f - (1.0f - tNorm) * (1.0f - tNorm);

			float boxW = baseBoxW * (0.88f + 0.12f * ease);
			float boxH = baseBoxH * (0.88f + 0.12f * ease);

			// Dim background scrim with smooth fade-in
			glColor4f(0.0f, 0.0f, 0.0f, 0.72f * ease);
			glBegin(GL_QUADS);
				glVertex2f(0, 0);
				glVertex2f(w, 0);
				glVertex2f(w, h);
				glVertex2f(0, h);
			glEnd();

			// Card base with glowing cyan border
			float borderPulse = 0.85f + 0.15f * std::sin(animTimer * 4.0f);
			drawPanel(cx - boxW * 0.5f, cy + boxH * 0.5f, boxW, boxH,
			          vec4(0.015f, 0.04f, 0.09f, 0.96f * ease),
			          vec4(0.0f, 0.85f * borderPulse, 1.0f * borderPulse, 0.95f), 2.5f);

			glColor3f(0.20f, 1.0f, 0.70f);
			drawString(cx - 100.0f, cy + 112.0f, "SECTOR DECOMPRESSION COMPLETE");

			glColor3f(1.0f, 0.88f, 0.20f);
			drawString(cx - 110.0f, cy + 86.0f, world->levelManager->getLevelName());

			const auto& res = world->levelManager->getCurrentResult();

			char timeBuf[64];
			sprintf_s(timeBuf, "COMPLETION TIME:  %.1f SECONDS   ( -%d pts )", res.timeTaken, res.timePenalty);
			glColor3f(0.85f, 0.95f, 1.0f);
			drawString(cx - 190.0f, cy + 50.0f, timeBuf);

			char cleanBuf[64];
			sprintf_s(cleanBuf, "CLEAN RUN BONUS:  +%d pts   (%d Collisions)", res.noCollisionBonus, res.collisions);
			if (res.noCollisionBonus > 0) glColor3f(0.35f, 1.0f, 0.55f);
			else glColor3f(0.65f, 0.70f, 0.75f);
			drawString(cx - 190.0f, cy + 24.0f, cleanBuf);

			char retryBuf[64];
			sprintf_s(retryBuf, "NO-RETRY BONUS:   +%d pts   (%s)", res.noRestartBonus, res.restarted ? "Restarted" : "First Try");
			if (res.noRestartBonus > 0) glColor3f(0.35f, 1.0f, 0.55f);
			else glColor3f(0.65f, 0.70f, 0.75f);
			drawString(cx - 190.0f, cy - 2.0f, retryBuf);

			char finBuf[64];
			sprintf_s(finBuf, "FINAL SECTOR SCORE:  %d POINTS", res.finalScore);
			glColor3f(1.0f, 0.90f, 0.25f);
			drawString(cx - 190.0f, cy - 36.0f, finBuf);

			char totBuf[64];
			sprintf_s(totBuf, "CUMULATIVE MISSION SCORE:  %d POINTS", world->levelManager->getTotalScore());
			glColor3f(0.20f, 1.0f, 0.85f);
			drawString(cx - 190.0f, cy - 62.0f, totBuf);

			glColor3f(1.0f, 1.0f, 1.0f);
			drawString(cx - 215.0f, cy - 105.0f, "PRESS [ENTER] / [N] NEXT LEVEL    |    [R] REPLAY    |    [Q] QUIT");
		}
		// B. GAME COMPLETE MODAL
		else if (st == LevelManager::GameState::GAME_COMPLETE) {
			float cx = w * 0.5f;
			float cy = h * 0.5f;
			const float baseBoxW = 620.0f;
			const float baseBoxH = 390.0f;

			float animTimer = world->levelManager->getLevelCompleteAnimTimer();
			float tNorm = (animTimer < 0.35f) ? (animTimer / 0.35f) : 1.0f;
			float ease = 1.0f - (1.0f - tNorm) * (1.0f - tNorm);

			float boxW = baseBoxW * (0.88f + 0.12f * ease);
			float boxH = baseBoxH * (0.88f + 0.12f * ease);

			glColor4f(0.0f, 0.0f, 0.0f, 0.80f * ease);
			glBegin(GL_QUADS);
				glVertex2f(0, 0);
				glVertex2f(w, 0);
				glVertex2f(w, h);
				glVertex2f(0, h);
			glEnd();

			float borderPulse = 0.85f + 0.15f * std::sin(animTimer * 4.0f);
			drawPanel(cx - boxW * 0.5f, cy + boxH * 0.5f, boxW, boxH,
			          vec4(0.015f, 0.04f, 0.09f, 0.96f * ease),
			          vec4(1.0f, 0.85f * borderPulse, 0.25f, 0.95f), 2.5f);

			glColor3f(1.0f, 0.88f, 0.20f);
			drawString(cx - 180.0f, cy + 165.0f, "MISSION ACCOMPLISHED // GRAVITY SHIFT MASTERED!");

			char totBuf[100];
			sprintf_s(totBuf, "FINAL TOTAL SCORE: %d POINTS   |   TOTAL TIME: %.1fs",
			          world->levelManager->getTotalScore(),
			          world->levelManager->getTotalCompletedTime());
			glColor3f(0.25f, 1.0f, 0.85f);
			drawString(cx - 210.0f, cy + 130.0f, totBuf);

			glColor3f(0.60f, 0.85f, 1.0f);
			drawString(cx - 280.0f, cy + 85.0f, "SECTOR                        TIME     PENALTY   BONUSES     SCORE");

			glColor4f(0.20f, 0.85f, 1.0f, 0.60f);
			glBegin(GL_LINES);
				glVertex2f(cx - 290.0f, cy + 75.0f);
				glVertex2f(cx + 290.0f, cy + 75.0f);
			glEnd();

			float rowY = cy + 50.0f;
			const char* lvlNames[5] = {
				"Level 1: Basic Movement ",
				"Level 2: Wall Walking   ",
				"Level 3: Crate Puzzle   ",
				"Level 4: Keycard        ",
				"Level 5: Gravity Master "
			};

			for (int i = 1; i <= 5; i++) {
				const auto& rec = world->levelManager->getRecord(i);
				char rowBuf[128];
				int totalBonus = rec.noCollisionBonus + rec.noRestartBonus;
				sprintf_s(rowBuf, "%s     %04.1fs    -%03d pts   +%03d pts    %04d pts",
				          lvlNames[i - 1],
				          rec.timeTaken,
				          rec.timePenalty,
				          totalBonus,
				          rec.finalScore);

				if (rec.finalScore >= 1000) glColor3f(0.35f, 1.0f, 0.60f);
				else glColor3f(1.0f, 0.90f, 0.40f);

				drawString(cx - 280.0f, rowY, rowBuf);
				rowY -= 25.0f;
			}

			glColor4f(0.20f, 0.85f, 1.0f, 0.60f);
			glBegin(GL_LINES);
				glVertex2f(cx - 290.0f, rowY + 5.0f);
				glVertex2f(cx + 290.0f, rowY + 5.0f);
			glEnd();

			glColor3f(1.0f, 0.85f, 0.20f);
			drawString(cx - 200.0f, cy - 145.0f, "PRESS [R] TO REPLAY FULL MISSION    |    [Q] TO EXIT GAME");
		}
		// C. OXYGEN DEPLETED MODAL
		else if (st == LevelManager::GameState::OXYGEN_DEPLETED) {
			float cx = w * 0.5f;
			float cy = h * 0.5f;
			const float boxW = 500.0f;
			const float boxH = 160.0f;

			glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
			glBegin(GL_QUADS);
				glVertex2f(0, 0);
				glVertex2f(w, 0);
				glVertex2f(w, h);
				glVertex2f(0, h);
			glEnd();

			drawPanel(cx - boxW * 0.5f, cy + boxH * 0.5f, boxW, boxH,
			          vec4(0.18f, 0.02f, 0.02f, 0.95f),
			          vec4(1.0f, 0.20f, 0.20f, 0.95f), 2.5f);

			glColor3f(1.0f, 0.25f, 0.25f);
			drawString(cx - 155.0f, cy + 40.0f, "CRITICAL ALERT: OXYGEN SUPPLY DEPLETED!");

			glColor3f(1.0f, 0.85f, 0.85f);
			drawString(cx - 175.0f, cy + 10.0f, "LIFE SUPPORT FAILED BEFORE REACHING ESCAPE POD.");

			glColor3f(1.0f, 0.90f, 0.30f);
			drawString(cx - 170.0f, cy - 30.0f, "PRESS [R] TO RETRY SECTOR 5    |    [Q] TO QUIT");
		}
	}

	// -------------------------------------------------------------
	// 6. BOTTOM CONTROLS GUIDE BAR
	// -------------------------------------------------------------
	drawPanel(padX, 38.0f, w - padX * 2.0f, 28.0f, vec4(0.012f, 0.035f, 0.08f, 0.90f), vec4(0.0f, 0.85f, 1.0f, 0.85f), 1.4f);

	glColor3f(0.70f, 0.92f, 1.0f);
	drawString(padX + 12.0f, 19.0f, "CONTROLS:  [W/S/A/D] Walk   [SPACE] Jump   [G] Shift Gravity   [E] Interact   [R] Restart Sector   [1-5] Warp Sector   [Q] Quit");

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	if (mainProgram != 0) {
		glUseProgram(mainProgram);
	}
}

void init()
{
	srand(GetTickCount());
	mainProgram = LoadShader("./source/vshader.glsl", "./source/fshader.glsl");
	glUseProgram(mainProgram);

	world = new World(mainProgram);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.02f, 0.025f, 0.04f, 1.f);
}

/**
 * @brief Main OpenGL Display Callback.
 * 
 * Executes a two-pass rendering pipeline:
 * 1. 3D Scene Pass: Clears color and depth buffers, renders room architecture,
 *    crates, portal, and player character with 3D model-view-projection transformations
 *    and hardware depth testing (GL_DEPTH_TEST).
 * 2. 2D HUD Pass: Disables depth writing, sets up an orthographic pixel projection (glOrtho),
 *    and renders responsive HUD panels, gravity icons, and modal dialogue overlays
 *    using alpha blending (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).
 * 3. Buffer Swap: Swaps double-buffered front and back frames (glutSwapBuffers).
 */
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (world) world->draw();
	renderHUD();
	glutSwapBuffers();
}

void reshape(int w, int h) {
	if (h <= 0) h = 1;
	glViewport(0, 0, w, h);
	float aspect = (float)w / (float)h;
	if (world) {
		world->setProjection(aspect);
	}
}

void keyboard(unsigned char key, int x, int y)
{
	unsigned char lower = (unsigned char)tolower(key);
	keys[lower] = true;
	keys[key] = true;

	// Check if gameplay is paused for modal completion screens
	if (world && world->levelManager) {
		LevelManager::GameState state = world->levelManager->getState();

		if (state == LevelManager::GameState::LEVEL_COMPLETE) {
			if (key == 13 || lower == 'n') {
				world->levelManager->proceedToNextLevel();
				if (world->player) {
					world->player->spawnPosition = world->levelManager->getPlayerSpawn();
					world->player->resetToSpawn();
				}
				return;
			}
			else if (lower == 'r') {
				world->levelManager->replayCurrentLevel();
				if (world->player) {
					world->player->spawnPosition = world->levelManager->getPlayerSpawn();
					world->player->resetToSpawn();
				}
				return;
			}
			else if (lower == 'q') {
				exit(0);
			}
			return; // Swallow other inputs while modal is displayed
		}
		else if (state == LevelManager::GameState::GAME_COMPLETE) {
			if (lower == 'r') {
				world->levelManager->replayGame();
				if (world->player) {
					world->player->spawnPosition = world->levelManager->getPlayerSpawn();
					world->player->resetToSpawn();
				}
				return;
			}
			else if (lower == 'q') {
				exit(0);
			}
			return;
		}
		else if (state == LevelManager::GameState::OXYGEN_DEPLETED) {
			if (lower == 'r') {
				world->levelManager->replayCurrentLevel();
				if (world->player) {
					world->player->spawnPosition = world->levelManager->getPlayerSpawn();
					world->player->resetToSpawn();
				}
				return;
			}
			else if (lower == 'q') {
				exit(0);
			}
			return;
		}
	}

	// Normal Playing Keys
	if (world && world->levelManager) {
		world->levelManager->dismissLevelIntro();
	}

	switch (lower) {
	case 'g':
		if (world) {
			world->cycleGravity();
		}
		break;
	case 'e':
		if (world) {
			world->interact();
		}
		break;
	case ' ':
		if (world && world->player) {
			world->player->jump();
		}
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		if (world && world->levelManager) {
			world->levelManager->loadLevel(lower - '0');
			if (world->player) {
				world->player->spawnPosition = world->levelManager->getPlayerSpawn();
				world->player->resetToSpawn();
			}
		}
		break;
	case 'r':
		if (world) {
			world->restartRoom();
		}
		break;
	case 'c':
		if (world) {
			world->toggleCeiling();
		}
		break;
	case 't':
		if (world && world->camera) {
			world->camera->currMode = Camera::VIEW_MODE::TP;
		}
		break;
	case 'f':
		if (world && world->camera) {
			world->camera->currMode = Camera::VIEW_MODE::FP;
		}
		break;
	case 'v':
		if (world && world->camera) {
			if (world->camera->currMode != Camera::TOP) {
				world->camera->prevMode = world->camera->currMode;
				world->camera->currMode = Camera::TOP;
			} else {
				world->camera->currMode = world->camera->prevMode;
			}
		}
		break;
	case 27: // ESC key
		if (world) {
			world->worldState = (world->worldState == World::RUN) ? World::PAUSE : World::RUN;
		}
		break;
	case 'q':
		exit(0);
		break;
	}
}

void keyboardUp(unsigned char key, int x, int y)
{
	unsigned char lower = (unsigned char)tolower(key);
	keys[lower] = false;
	keys[key] = false;
}

void special(int key, int x, int y) {
	if (key >= 0 && key < 256) {
		specialKeys[key] = true;
	}
}

void specialUp(int key, int x, int y) {
	if (key >= 0 && key < 256) {
		specialKeys[key] = false;
	}
}

void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
		if (world->worldState == World::RUN) {
			world->worldState = World::PAUSE;
		} else if (world->worldState == World::PAUSE) {
			world->worldState = World::RUN;
		}
	}
	if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN)) {
		if (world->worldState == World::RUN) {
			world->worldState = World::PAUSE;
		} else if (world->worldState == World::PAUSE) {
			world->worldState = World::ONE_FRAME;
		}
	}
}

/**
 * @brief Fixed-step game simulation and physics timer callback (~60 FPS).
 * 
 * Computes elapsed delta time (dt) clamped between 2ms and 50ms for numerical stability.
 * Gathers user input (WASD walking), executes kinematic integration, updates collisions,
 * advances animations, requests window redisplay (glutPostRedisplay), and reschedules
 * the timer callback for ~60 FPS (16.67ms).
 */
void timer(GLint) {
	static DWORD lastTime = 0;
	DWORD now = GetTickCount();
	if (lastTime == 0) lastTime = now - 16;
	float dt = (now - lastTime) / 1000.0f;
	lastTime = now;
	if (dt < 0.002f) dt = 0.016f;
	if (dt > 0.050f) dt = 0.050f;

	if (world && world->player && world->worldState != World::PAUSE) {
		bool isPlaying = (world->levelManager == nullptr || world->levelManager->getState() == LevelManager::GameState::PLAYING);

		if (isPlaying) {
			float fwd = 0.0f;
			float strafe = 0.0f;

			// W / S: Walk Forward / Backward relative to camera and surface
			if (keys['w']) fwd += 1.0f;
			if (keys['s']) fwd -= 1.0f;

			// A / D: Strafe Left / Right
			if (keys['a']) strafe -= 1.0f;
			if (keys['d']) strafe += 1.0f;

			// Walk with responsive deceleration
			world->player->walk(fwd, strafe, dt);
		} else {
			world->player->velocity = vec3(0.0f, 0.0f, 0.0f);
			world->player->currentSpeed = 0.0f;
		}

		world->updateWorld(dt);
	}

	glutPostRedisplay();
	glutTimerFunc(1000.f / 60.f, timer, 0); // 60 FPS
}

LONG WINAPI CustomCrashHandler(EXCEPTION_POINTERS* pExceptionInfo) {
	FILE* f = nullptr;
	fopen_s(&f, "crash.log", "w");
	if (f) {
		fprintf(f, "CRASH: ExceptionCode = 0x%08X at Address = 0x%p\n",
			pExceptionInfo->ExceptionRecord->ExceptionCode,
			pExceptionInfo->ExceptionRecord->ExceptionAddress);
		fclose(f);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL WINAPI ConsoleHandler(DWORD ctrlType) {
	return TRUE;
}

int main(int argc, char **argv) {
	SetUnhandledExceptionFilter(CustomCrashHandler);
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(960, 780);
	glutCreateWindow("GRAVITY SHIFT");

	glewExperimental = GL_TRUE;
	glewInit();

	init();

	glutTimerFunc(1000.f / 60.f, timer, 0);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(special);
	glutSpecialUpFunc(specialUp);
	glutIgnoreKeyRepeat(1);

	glutMainLoop();
	return 0;
}