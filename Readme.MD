# Gravity Shift

A 3D Gravity-Switching Puzzle Platformer developed in **C++** and **OpenGL** (GLSL 1.50 Shader Pipeline + Compatibility Profile HUD).

---

## 1. Project Overview

**Gravity Shift** is a 3D puzzle-platformer where the player navigates an astronaut through a modular futuristic space research station. By manipulating localized artificial gravity across four cardinal axes (Floor, Left Wall, Ceiling, and Right Wall), the player must overcome architectural obstacles, push heavy cargo crates, collect security keycards, disable energy forcefields, and reach extraction portals.

The game is engineered for evaluation in an academic Computer Graphics context (CIE-3), demonstrating:
- Programmable shader pipelines (GLSL 1.50).
- Blinn-Phong per-fragment illumination with quadratic distance attenuation.
- Composite 3D geometric modeling and procedural mesh generation.
- Model, View, and Perspective Projection transformation mathematics.
- Dynamic camera systems with horizon alignment across multiple gravity axes.
- Kinematic surface-tangent movement, AABB collision detection, and impulse momentum transfer.
- Dual-pass rendering combining programmable 3D shaders with a fixed-function 2D HUD overlay.

---

## 2. Required Software & Libraries

| Dependency | Version / Standard | Purpose |
| :--- | :--- | :--- |
| **Operating System** | Windows 10 / 11 (x86/x64) | Target platform |
| **Compiler / IDE** | MSVC v141 / v142 / v143 / v145 (Visual Studio 2017+) | C++14/C++17 build environment |
| **OpenGL** | 3.2+ Compatibility Profile (GLSL 1.50) | GPU-accelerated 3D scene rendering & fixed-function 2D HUD overlay |
| **GLEW** | 2.1.0 | OpenGL extension wrangler (`glew32.dll`) |
| **FreeGLUT** | 3.0.0 | Window creation, double buffering, input dispatch (`freeglut.dll`) |
| **FMOD Core API** | 1.10+ | Low-latency 3D audio playback and ambient streaming (`fmod.dll`) |
| **Angel Math Library**| Custom header-only (`vec.h`, `mat.h`) | Homogeneous matrix and vector operations |

*Note: GLEW and FreeGLUT dependencies are automatically restored via NuGet (`nupengl.core.0.1.0.1`) during build.*

---

## 3. Build & Run Instructions

### Option A: Visual Studio IDE
1. Open [`GravityShift.sln`](file:///c:/Users/hp/OneDrive/Desktop/Gravity%20game/GravityShift.sln) in Visual Studio.
2. Select **Release** configuration and **x86** (Win32) platform from the top toolbar.
3. Build the solution using **Build > Build Solution** (`Ctrl+Shift+B`).
4. Run the game via **Debug > Start Without Debugging** (`Ctrl+F5`).

### Option B: Command Line (MSBuild)
From a developer command prompt or PowerShell:
```powershell
# Restore packages and compile in Release x86 mode
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" GravityShift.sln /p:Configuration=Release /p:Platform=x86 /t:Build

# Run executable directly
.\Release\GravityShift.exe
```

### Option C: Automated Verification Harnesses
```powershell
# Run the complete 5-level automated verification suite
powershell -ExecutionPolicy Bypass -File .\verify_all_levels.ps1

# Run the 9-phase integration regression test suite
powershell -ExecutionPolicy Bypass -File .\test_gravity_shift.ps1
```

*Prerequisite: Ensure `fmod.dll`, `freeglut.dll`, `glew32.dll`, `source/vshader.glsl`, `source/fshader.glsl`, and the `media/` directory reside in the execution directory.*

---

## 4. Controls

| Input | Action | Description |
| :---: | :--- | :--- |
| **`W` / `S`** | Walk Forward / Backward | Moves character along active surface tangent (+Y/-Y) |
| **`A` / `D`** | Strafe Left / Right | Moves character laterally along active surface tangent |
| **`SPACE`** | Jump | Imparts vertical impulse along active surface normal |
| **`G`** | Shift Gravity | Cycles gravity sequentially across cardinal surfaces (Floor → Left Wall → Ceiling → Right Wall) |
| **`E`** | Interact | Pushes cargo crates, collects keycard, triggers exit portal |
| **`R`** | Restart Sector | Resets player and puzzle state (invalidates No-Restart bonus) |
| **`T`** | Third-Person View | Default tracking camera behind and above character |
| **`F`** | First-Person View | Interior helmet visor perspective |
| **`V`** | Top-Down View | Tactical orthographic-style overview of sector |
| **`C`** | Toggle Ceiling | Toggles ceiling mesh rendering for open visual inspection |
| **`1` – `5`** | Sector Warp | Direct level selection shortcut (Evaluator debug tool) |
| **`ESC` / `LMB`**| Pause / Resume | Pauses world physics and animation timers |
| **`Q`** | Quit Game | Cleanly releases resources and terminates application |

*Note on Gravity Switching: Gravity directions are not restricted per level. Pressing `G` cycles through all four cardinal axes at any time; however, each sector's layout and hazard configuration incentivize specific navigational paths.*

---

## 5. Description of All Five Levels

### Sector 1: Basic Movement & Calibration
- **Objective:** Navigate the linear corridor to reach the extraction portal.
- **Surface Traversal:** Baseline floor movement.
- **Mechanics:** Baseline calibration of WASD kinematic walking, camera tracking, boundary wall collision detection, and exit portal proximity detection with `[E]` interaction.

### Sector 2: Wall Walking
- **Objective:** Shift gravity to the left bulkhead to bypass the electrified floor barrier and reach the exit portal.
- **Surface Traversal:** Shift gravity to the Left Wall (`GRAV_LEFT_WALL`).
- **Mechanics:** An electrified hazard field spans the center floor ($-7.8 \le X \le 7.8, -8.0 \le Y \le 14.0$). Pressing `G` rotates gravity toward the left wall ($-X$). The player accelerates toward the wall, aligns their local up-vector with the left bulkhead, walks along the open wall corridor, and safely reaches the exit door.

### Sector 3: Crate Puzzle
- **Objective:** Dislodge a heavy cargo crate obstructing the portal aperture.
- **Surface Traversal:** Floor traversal.
- **Mechanics:** The exit portal is blocked by a 1.2m metallic crate (`isExitBlocked() == true` while $|X| < 1.9$ and $Y > 17.0$). The player must walk into the crate or press `E` to transfer momentum, sliding the crate forward and aside off the doorway threshold to unlock passage.

### Sector 4: Find the Keycard
- **Objective:** Retrieve the security keycard from the ceiling to disable the doorway forcefield barrier.
- **Surface Traversal:** Shift gravity to the Ceiling (`GRAV_CEILING`).
- **Mechanics:** Direct portal access is barred by a locked blast forcefield barrier across the doorway ($Y \approx 21.0$). The holographic keycard is mounted high on the ceiling at $(0, 0, 7.30)$. The player shifts gravity to the ceiling, approaches the keycard to collect it (`[E]`, distance $< 2.5\text{m}$), which deactivates the forcefield and permits sector exit.

### Sector 5: Gravity Master / Evacuation
- **Objective:** Retrieve the wall keycard, push aside the cargo crate, and reach the escape hatch before oxygen depletes.
- **Surface Traversal:** Multi-surface traversal utilizing Floor, Left Wall, Ceiling, and Right Wall.
- **Mechanics:** Evacuation emergency with an active 65-second life-support oxygen countdown. The floor contains a forward hazard field, the security keycard is mounted on the Right Wall ($X = 9.25, Y = 6.0, Z = 4.0$), and a movable cargo crate blocks the doorway threshold ($Y = 17.8$). The player must dynamically cycle gravity across bulkheads to bypass the hazard, collect the keycard, dislodge the crate, and escape before oxygen runs out.

---

## 6. Implemented Computer Graphics Concepts

### 6.1 Programmable Shader Pipeline
Rendering utilizes GLSL 1.50 shaders ([`source/vshader.glsl`](file:///c:/Users/hp/OneDrive/Desktop/Gravity%20game/source/vshader.glsl) and [`source/fshader.glsl`](file:///c:/Users/hp/OneDrive/Desktop/Gravity%20game/source/fshader.glsl)):
- **Vertex Shader (`#version 150`):** Transforms vertex coordinates from object space to homogeneous clip space ($P \cdot V \cdot M \cdot v$). Computes eye-space vertex position ($pos$) and transformed normal ($N = (M_{\text{view}} \cdot \text{vec4}(v_{\text{normal}}, 0.0)).xyz$).
- **Fragment Shader (`#version 150`):** Evaluates per-fragment Blinn-Phong lighting, calculating ambient, diffuse, and specular illumination from point light sources with distance attenuation ($1 / (1 + 0.024 d + 0.0018 d^2)$), secondary omnidirectional fill lighting, Fresnel silhouette rim lighting, and atmospheric spatial depth cueing (fog).

### 6.2 Blinn-Phong Illumination Model
Per-fragment lighting follows the classic Phong reflectance formula:
$$I = I_{\text{ambient}} + I_{\text{diffuse}} + I_{\text{specular}}$$
$$I = K_a L_a + K_d L_d \max(\vec{N} \cdot \vec{L}, 0) + K_s L_s \max(\vec{N} \cdot \vec{H}, 0)^\alpha$$
where:
- $\vec{N}$ is the surface normal in eye space.
- $\vec{L}$ is the normalized light vector from fragment to light source.
- $\vec{H} = \frac{\vec{L} + \vec{V}}{\|\vec{L} + \vec{V}\|}$ is the Blinn half-vector between light and viewer.
- $\alpha$ is the material shininess exponent.

Multi-material rendering sets distinct optical properties for each mesh component:
- **Suit Fabric:** Matte composite white ($K_d = [0.85, 0.88, 0.92]$, $\alpha = 35$).
- **Helmet Visor:** High-specular reflective cyan glass ($K_s = [0.95, 1.0, 1.0]$, $\alpha = 95$).
- **Joints / Backpack:** Dark graphite steel ($K_d = [0.24, 0.26, 0.30]$, $\alpha = 50$).
- **Safety Accents:** Safety orange ($K_d = [1.0, 0.58, 0.18]$, $\alpha = 65$).

### 6.3 Coordinate Transformation Pipeline
All scene objects follow the standard graphics pipeline:
1. **Model Matrix ($M$):**
   $$M = T(\vec{p}) \cdot R_{\text{surface}} \cdot R_z(\theta_{\text{yaw}})$$
   Synthesizes translation to world position $\vec{p}$, smooth surface orientation rotation $R_{\text{surface}}$, and yaw heading angle $\theta_{\text{yaw}}$.
2. **View Matrix ($V$):**
   $$V = \text{LookAt}(\vec{e}_{\text{eye}}, \vec{a}_{\text{at}}, \vec{u}_{\text{up}})$$
   Dynamically aligns camera up-vector $\vec{u}_{\text{up}}$ with active gravity normal to maintain a stable horizon without gimbal flipping.
3. **Projection Matrix ($P$):**
   $$P = \text{Perspective}(\text{fovy} = 60^\circ, \text{aspect} = w/h, z_{\text{near}} = 0.20\text{m}, z_{\text{far}} = 150.0\text{m})$$

### 6.4 Dual-Pass Rendering Architecture (Hybrid Pipeline)
[`source/Main.cpp`](file:///c:/Users/hp/OneDrive/Desktop/Gravity%20game/source/Main.cpp) implements a dual-pass rendering architecture combining modern programmable shaders with fixed-function OpenGL:
1. **Pass 1 (3D World Pass - Programmable Pipeline):** Shader pipeline with depth testing enabled (`GL_DEPTH_TEST`), perspective projection active, and bound VAO/VBOs. Renders the research room bulkheads, hazard obstacles, crates, keycard crystal, and player mesh with GLSL 1.50 Blinn-Phong lighting.
2. **Pass 2 (2D HUD Overlay Pass - Fixed-Function Pipeline):** Shaders and VBOs are unbound (`glUseProgram(0)`), depth buffer writes and testing are disabled (`glDisable(GL_DEPTH_TEST)`), and fixed-function OpenGL is engaged:
   - Matrix stack manipulation: `glMatrixMode(GL_PROJECTION)` with `glOrtho(0, w, 0, h, -1, 1)` and `glMatrixMode(GL_MODELVIEW)`.
   - Immediate-mode primitive rasterization: `glBegin(GL_QUADS)` / `glBegin(GL_LINE_LOOP)` / `glEnd()` with `glColor4fv()`.
   - Raster text output: `glRasterPos2f()` and FreeGLUT bitmap fonts via `glutBitmapCharacter()`.
   
   Because the HUD relies on fixed-function matrix stacks and immediate-mode calls (which are removed in a strict Core Profile), the application runs under an OpenGL Compatibility Profile context.

### 6.5 Procedural Mesh Construction
All geometry is generated procedurally in memory and streamed to GPU VBOs:
- **Astronaut:** Parametric UV spheres (helmet, joints), 3D extruded boxes (torso, limbs), and open cylinders (oxygen canisters).
- **Research Room:** Modular bulkheads, recessed wall lights, and hexagonal support pillars.
- **Crates & Obstacles:** 6-sided boxes with explicit outward face normals.

---

## 7. Physics and Collision Approach

### 7.1 Kinematic Surface Walking
Player movement is integrated relative to the active surface tangent coordinate frame:
- **Floor:** $\vec{t}_{\text{fwd}} = [0, 1, 0]$, $\vec{t}_{\text{strafe}} = [1, 0, 0]$
- **Left Wall:** $\vec{t}_{\text{fwd}} = [0, 1, 0]$, $\vec{t}_{\text{strafe}} = [0, 0, 1]$
- **Ceiling:** $\vec{t}_{\text{fwd}} = [0, 1, 0]$, $\vec{t}_{\text{strafe}} = [-1, 0, 0]$
- **Right Wall:** $\vec{t}_{\text{fwd}} = [0, 1, 0]$, $\vec{t}_{\text{strafe}} = [0, 0, -1]$

Velocity integrates responsive acceleration when input is active ($\text{rate} = 38.0\,\text{m/s}^2$) and Coulomb friction deceleration to rest when released ($\text{friction} = 18.0\,\text{s}^{-1}$), eliminating floaty sliding.

### 7.2 Gravity Vector & Orientation Interpolation
Gravity acceleration vector is mapped per active surface:
$$\vec{g}_{\text{floor}} = [0, 0, -20.0], \quad \vec{g}_{\text{ceiling}} = [0, 0, +20.0], \quad \vec{g}_{\text{left}} = [-20.0, 0, 0], \quad \vec{g}_{\text{right}} = [+20.0, 0, 0]$$

When switching gravity, target surface orientation matrix $R_{\text{target}}$ is smoothly interpolated using a cubic smoothstep function ($S(t) = 3t^2 - 2t^3$) over 180ms, providing clean visual reorientation without jarring cuts.

### 7.3 AABB Collision & Penetration Resolution
- **Boundaries:** Continuous AABB testing against room planes: Floor ($Z=0$), Ceiling ($Z=8$), Left Wall ($X=-10$), Right Wall ($X=10$), Rear Wall ($Y=-22$), Front Wall ($Y=24$).
- **Response:** Penetration is resolved by clamping character position along the contact normal and canceling the normal velocity component ($v_n = 0$).
- **Grounded State:** `isGrounded` is confirmed when surface contact opposes the active gravity vector.

### 7.4 Dynamic Crate Impulse & Friction
Crates follow semi-implicit Euler integration:
$$\vec{p}_{\text{crate}} \mathrel{+}= \vec{v}_{\text{crate}} \cdot \Delta t$$
$$\vec{v}_{\text{crate}} \mathrel{*}= (1.0 - 2.8 \Delta t)$$
On player-crate AABB overlap along the minimum penetration axis, the crate receives an immediate velocity impulse ($\vec{v}_{\text{crate}} \mathrel{+}= 2.5\,\text{m/s}$), and the player's forward momentum is halted.

### 7.5 Scoring & Performance System
Level completion score is computed via multi-factor evaluation:
$$\text{Score} = \max\left(0, \, 1000 - 10 \cdot \lfloor t_{\text{elapsed}} \rfloor + B_{\text{collisions}} + B_{\text{restart}}\right)$$
- **Base Score:** 1,000 points.
- **Time Penalty:** -10 points per full elapsed second.
- **Zero-Collision Bonus ($B_{\text{collisions}}$):** +100 points if collision count is zero.
- **Clean Run Bonus ($B_{\text{restart}}$):** +100 points if completed on the first attempt without restarting.

---

## 8. Project Structure

```
Gravity game/
├── GravityShift.sln          # Master Visual Studio 2017+ Solution
├── GravityShift.vcxproj      # MSBuild C++ Project Configuration
├── GravityShift.vcxproj.filters # Solution Explorer Filter Hierarchy
├── GravityShift.vcxproj.user # Local Debugging Preferences
├── packages.config           # NuGet Package Specification (GLEW, FreeGLUT)
├── test_gravity_shift.ps1    # 9-Phase Automated Integration Test Harness
├── verify_all_levels.ps1     # 5-Level End-to-End Verification Harness
├── Readme.MD                 # Project Documentation & Technical Overview
│
├── include/                  # C++ Header Declarations
│   ├── Player.h              # Astronaut Character Model & Physics
│   ├── Camera.h              # Dynamic Tracking Camera (TP/FP/TOP)
│   ├── Room.h                # Research Station Architecture Geometry
│   ├── LevelManager.h        # 5-Level Configuration, Crate Physics & Scoring
│   ├── World.h               # Central Simulation & Audio Coordinator
│   ├── Materials.h           # Optical Material Properties for Lighting
│   ├── Angel/                # Mathematical Utility Library
│   │   ├── Angel.h           # Master Include Header
│   │   ├── vec.h             # 2D, 3D, 4D Vector Classes
│   │   └── mat.h             # 2x2, 3x3, 4x4 Transformation Matrices
│   └── FMOD/                 # FMOD Sound System API Headers
│
├── source/                   # C++ Implementation Files & Shaders
│   ├── Main.cpp              # Entry Point, Game Loop, 2D HUD Pass, Input
│   ├── Player.cpp            # Procedural Astronaut Mesh, Kinematics, Gravity
│   ├── Camera.cpp            # LookAt View Matrix & Horizon Stabilization
│   ├── Room.cpp              # Modular Architecture, Lights, Pillar Geometry
│   ├── LevelManager.cpp      # Level Loading, Crate Physics, Score Records
│   ├── World.cpp             # Simulation Loop, AABB Collision, Audio
│   ├── vshader.glsl          # Vertex Shader (ModelViewProjection & Normals)
│   └── fshader.glsl          # Fragment Shader (Blinn-Phong Illumination)
│
├── lib/                      # Linker Import Libraries
│   ├── fmod_vc.lib           # FMOD Core 32-bit Linker Library
│   └── fmod64_vc.lib         # FMOD Core 64-bit Linker Library
│
└── media/                    # Audio Sound Assets
    ├── ambience.wav          # Atmospheric Research Station Background Drone
    ├── engine_startup.wav    # Gravity Shift Activation Impulse Sound
    ├── blaster.wav           # Security Keycard Pickup Chime
    └── explosion.wav         # Heavy Cargo Crate Impact Sound
```

---

## 9. Known Limitations

1. **Cardinal Gravity Axes:** Gravity shifting is currently restricted to the four primary cardinal planes parallel to room bulkheads ($-\hat{Z}, +\hat{X}, +\hat{Z}, -\hat{X}$). Sloped surfaces (wedges/ramps) are not supported.
2. **Simplified Rigid-Body Physics:** Cargo crates utilize an impulse-momentum transfer model with Coulomb surface damping rather than a full multi-contact rigid body constraint solver.
3. **Fixed Window Resolution Defaults:** While the HUD and projection matrix dynamically adapt to arbitrary window aspect ratios, default starting resolution is tuned to 960x780.
