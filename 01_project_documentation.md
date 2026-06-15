# OpenGL Project Documentation — Space Escape (Asteroids & Black Hole)

> **Course:** Computacion Grafica 2026-I — UCSP
> **Team:** Alexander Baylon, Cristian Mellado, Jose Vilca, Walter Valdivia
> **Standard:** C++17
> **Libraries:** GLFW, GLAD, FreeImage, STB libraries

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [File Tree](#2-file-tree)
3. [Coding Style Rules](#3-coding-style-rules)
4. [Architecture Overview](#4-architecture-overview)
5. [Module Reference](#5-module-reference)
   - [main.cpp](#51-maincpp)
   - [vertex.h](#52-vertexh)
   - [matriz.h](#53-matrizh)
   - [transform.h](#54-transformh)
   - [helper.h](#55-helperh)
   - [figura.h](#56-figurah)
   - [3dsloader.h / .cpp](#57-3dsloaderh--cpp)
   - [camera.h](#58-camerah)
   - [spaceship.h](#59-spaceshiph)
   - [blackhole.h](#510-blackholeh)
   - [asteroid.h](#511-asteroidh)
   - [skybox.h](#512-skyboxh)
6. [Shader System](#6-shader-system)
7. [Input & Controls](#7-input--controls)
8. [Rendering Pipeline](#8-rendering-pipeline)
9. [Asset Inventory](#9-asset-inventory)
10. [Global State](#10-global-state)

---

## 1. Project Overview

This is a hybrid OpenGL 3.3 Core Profile project combining two interactive 3D experiences:

1. **Rubik's Cube** — A fully playable, animatable, solvable 3x3 Rubik's cube with slice rotations, scramble/solve sequences, and global transforms. Kept as a secondary scene element.

2. **Space Escape** — A spaceship game set in space where the player controls a 3D spaceship model, dodging and destroying procedurally spawned asteroids while navigating near a black hole with accretion disk, polar jets, and falling gas particles.

The scene contains:
- A **Rubik's cube** at the origin (decorative)
- A **player spaceship** (3DS model) that can fly through space
- A **black hole** with skybox nebula, event horizon sphere, accretion disk, photon ring, polar jets, and gas particles
- **Asteroids** that spawn dynamically and fly toward the player
- Two **camera modes**: orbital (free) and follow (chases the ship)

---

## 2. File Tree

```
Rubik_Project/
|
|-- main.cpp                    # Entry point, render loop, global state
|-- glad.c                      # GLAD OpenGL function loader
|
|-- vertex.h                    # vec3 and vec2 math classes
|-- matriz.h                    # Custom 4x4 matrix class
|-- transform.h                 # Transformation stack (translate, rotate, scale)
|-- helper.h                    # Utility functions, texture loading, math
|-- figura.h                    # Base Figure class + Cube subclass (for Rubik)
|-- 3dsloader.h / .cpp          # 3DS file format parser
|-- camera.h                    # Camera with orbital + follow modes
|-- spaceship.h                 # Player spaceship (3DS model, movement)
|-- blackhole.h                 # Black hole system (skybox, disk, jets, particles)
|-- asteroid.h                  # Asteroid enemy system (shared mesh)
|-- skybox.h                    # Generic skybox renderer (cube map)
|-- rubik.h                     # Full Rubik's cube implementation (not documented here)
|
|-- stb_image.cpp / .h          # STB image loading library
|
|-- CMakeLists.txt              # Build configuration
|
|-- assets/
|   |-- spaceship.3DS           # Player ship model
|   |-- spaceshiptexture.bmp    # Ship diffuse texture
|   |-- asteroide.3ds           # Asteroid model
|   |-- asteroide.jpg           # Asteroid texture
|   |-- cubitoBorder.png        # Rubik cube face texture
|   |-- bh_colormap.png         # Black hole color map
|   |-- uv_checker.png          # UV debug texture
|   |
|   |-- bh_nebula/              # Black hole skybox (cube map)
|   |   |-- right.png
|   |   |-- left.png
|   |   |-- top.png
|   |   |-- bottom.png
|   |   |-- front.png
|   |   |-- back.png
|   |
|   |-- stars/                  # Alternative star skybox (currently unused)
|       |-- front.jpg
|       |-- back.jpg
|       |-- left.jpg
|       |-- right.jpg
|       |-- top.jpg
|       |-- bottom.jpg
|
|-- solver/                     # Kociemba Rubik solver (external)
|   |-- solve.h / .cpp
|   |-- facecube.h / .cpp
|   |-- cubiecube.h / .cpp
|   |-- coordcube.h / .cpp
|   |-- prunetable_helpers.h / .cpp
|   |-- random.h / .cpp
|   |-- search.h / .cpp
|   |-- color.h, corner.h, edge.h, facelet.h
```

---

## 3. Coding Style Rules

| Rule | Convention |
|------|-----------|
| Functions & variables | `snake_case` |
| Classes | `Pascal_snake_case` (e.g., `Cubo_rubik`) |
| Booleans | Named as yes/no questions: `is_empty`, `is_repeating`, `is_animation_running` |
| Related constants | Prefer `enum` over `#define` |
| Error handling | Verbose console output with `[MODULE]` prefixes |
| Matrices | Row-major order in `std::array<float,16>`, stored as `{m00,m01,m02,m03, m10,m11,...}` |
| OpenGL matrix uploads | `GL_TRUE` (transpose flag) because custom math is row-major |

---

## 4. Architecture Overview

```
main.cpp (render loop)
|
|-- Camera (orbital OR follow mode)
|   |-- getViewMatrix()          # spherical orbital camera
|   |-- getFollowViewMatrix()    # smoothed chase camera
|   |-- getPerspectiveMatrix()   # projection
|
|-- BlackHole (own shader programs)
|   |-- drawSkybox()             # nebula cube map
|   |-- drawSphere()             # event horizon (pure black)
|   |-- drawDisk()               # accretion disk particles
|   |-- drawPhotonRing()         # light ring around BH
|   |-- drawJets()               # polar emission jets
|   |-- drawGasParticles()       # falling gas (currently unused in draw)
|
|-- ShaderProgram (main: texture + vertex colors)
|   |-- CuboRubik::draw()        # 26 cubes, 6 faces each
|   |-- Asteroid::draw()         # shared VAO, instanced logic
|   |-- Spaceship::draw()        # 3DS model VAO
|
|-- Asteroid Spawner
|   |-- spawn every 1.5s at z=-50 toward ship position
|   |-- cleanup when z > 2
```

**Important:** There is NO lighting system currently. The fragment shader only does texture sampling + color blending. Normals are not computed or used anywhere.

---

## 5. Module Reference

### 5.1 main.cpp

**Purpose:** Entry point, GLFW initialization, render loop, shader compilation, global game state.

**Global variables:**

| Variable | Type | Purpose |
|----------|------|---------|
| `listaAsteroides` | `std::vector<Asteroid>` | Active asteroid instances |
| `tiempoUltimoAsteroide` | `float` | Timestamp of last asteroid spawn |
| `frecuenciaSpawn` | `float` | Spawn interval in seconds (1.5s) |
| `asteroideTexID` | `unsigned int` | OpenGL texture ID for asteroids |
| `camera` | `Camera` | Global camera instance |
| `cuboRubik` | `CuboRubik*` | Rubik cube instance (heap allocated) |
| `spaceship` | `Spaceship` | Player ship instance |
| `isClockwise` | `bool` | Rubik rotation direction toggle |
| `trans` | `Transform` | Temp transform (mostly unused) |
| `lastFrame` / `deltaTime` / `currentFrame` | `float` | Frame timing |
| `lastMouseX` / `lastMouseY` | `float` | Previous mouse position |
| `firstMouse` | `bool` | Mouse initialization flag |
| `mouseSensitivity` | `const float` | Mouse look sensitivity (0.1) |
| `backgroundColor` | `colorVec` | Clear color (black by default) |
| `currentDrawMode` | `GLenum` | Polygon mode (LINE, FILL, POINT) |

**Vertex Shader (`vertexShaderSource`):**
- Input: `aPos` (location 0), `aColor` (location 1), `aTexCoord` (location 2)
- Output: `ourColor`, `TexCoord`
- Uniforms: `model`, `view`, `projection`
- Transforms vertex: `gl_Position = projection * view * model * vec4(aPos, 1.0)`

**Fragment Shader (`fragmentShaderTexSource`):**
- Input: `ourColor`, `TexCoord`
- Uniform: `sampler2D ourTexture`
- Logic: If texture alpha < 0.1, use vertex color. Otherwise blend 70% texture + 30% vertex color.
- **No lighting calculations whatsoever.**

**Render loop order (per frame):**
1. Compute `deltaTime`
2. `processInput()` — continuous input (WASD, arrows)
3. `glfwSetKeyCallback()` — discrete input (key_callback)
4. Clear color + depth buffers
5. `camera.updateCameraAnimation(dt)`
6. `camera.setTarget(spaceship.getPosition())`
7. If follow mode: `camera.updateFollow(shipPos, yaw, pitch, dt)`
8. `cuboRubik->update_animation(dt)`
9. **Draw Black Hole** (uses its own shaders, draws first)
10. Activate main shader, upload view/proj matrices
11. **Draw Rubik's Cube** (binds cubitoBorder.png texture)
12. **Spawn asteroids** if interval elapsed
13. **Update + Draw asteroids** (binds asteroide texture)
14. **Draw spaceship** (binds spaceship texture)
15. Swap buffers, poll events

**Functions:**

| Function | Description |
|----------|-------------|
| `main()` | Full initialization + render loop |
| `framebuffer_size_callback()` | GL viewport resize |
| `key_callback()` | Discrete key events (press/release) |
| `processInput()` | Continuous per-frame input (movement) |
| `cursor_position_callback()` | Mouse look for spaceship yaw/pitch |
| `getRandomColor()` | Returns random RGB colorVec |

---

### 5.2 vertex.h

**Classes:** `vec3`, `vec2`

**`vec3`** — 3D vector with component-wise operators:
- `x, y, z` public floats
- Constructors: `vec3()`, `vec3(x,y,z)`
- Operators: `+`, `-`, `*`, `/`, `+=` (both `const` and non-const variants)
- Getters: `getX()`, `getY()`, `getZ()`
- Setters: `setX()`, `setY()`, `setZ()`
- Note: Has TWO `operator+` variants (one takes `vec3&`, one takes `const vec3&`) — historical duplication

**`vec2`** — 2D vector (texture coordinates, UVs):
- `x, y` public floats
- Same getter/setter pattern as vec3
- `operator+` with `std::vector<float>&` is **empty/unimplemented**

**Key issue:** No `dot`, `cross`, or `length` methods on the class itself — these live in `helper.h` as free functions. No `normalize` method on the class either.

---

### 5.3 matriz.h

**Class:** `matriz4x4`

**Storage:** `std::array<float, 16> mat` in **row-major** order:
```
mat[0..3]   = row 0:  m00, m01, m02, m03
mat[4..7]   = row 1:  m10, m11, m12, m13
mat[8..11]  = row 2:  m20, m21, m22, m23
mat[12..15] = row 3:  m30, m31, m32, m33
```

Default constructor initializes to identity.

**Methods:**

| Method | Description |
|--------|-------------|
| `multMat(const matriz4x4&)` | Matrix multiplication `this = this * other` (row-major) |
| `multFig(const std::vector<vec3>&)` | Transform a vector of vertices by this matrix |
| `ortonormalizar()` | Orthonormalize the rotation component (prevents skewing) |

**Matrix layout convention:** The matrix stores translation in `mat[3]`, `mat[7]`, `mat[11]` (last column of each row). When passed to OpenGL with `GL_TRUE` transpose, this becomes standard OpenGL column-major.

**Important:** The translation components are at indices 3, 7, 11 — NOT 12, 13, 14. This is because the array is row-major and the 4th column stores Tx, Ty, Tz.

---

### 5.4 transform.h

**Class:** `Transform`

A transformation stack system that tracks operations via stacks for undo capability. Each operation has a forward and inverse variant.

**Stacks:**
- `rotXpila`, `rotYpila`, `rotZpila` — rotation angle stacks (radians)
- `traslacionPila` — translation vector stack
- `escalaPila` — scale vector stack

**Public matrix:** `matriz4x4 m` — the accumulated transform

**Methods:**

| Method | Returns | Description |
|--------|---------|-------------|
| `traslacion(vec3)` | `matriz4x4` | Translation matrix, pushes to stack |
| `traslacion_i()` | `matriz4x4` | Inverse translation, pops from stack |
| `escala(vec3)` | `matriz4x4` | Scale matrix, pushes to stack |
| `escala_i()` | `matriz4x4` | Inverse scale, pops from stack |
| `rotacionX(float)` | `matriz4x4` | X rotation (degrees), pushes radians |
| `rotacionX_i()` | `matriz4x4` | Inverse X rotation, pops stack |
| `rotacionY(float)` | `matriz4x4` | Y rotation (degrees), pushes radians |
| `rotacionY_i()` | `matriz4x4` | Inverse Y rotation, pops stack |
| `rotacionZ(float)` | `matriz4x4` | Z rotation (degrees), pushes radians |
| `rotacionZ_i()` | `matriz4x4` | Inverse Z rotation, pops stack |
| `mult(const matriz4x4&)` | `Transform&` | Multiplies into internal matrix `m` |
| `dataPtr()` | `const float*` | Pointer to matrix data |
| `reset()` | `void` | Resets `m` to identity |

**Macros:** `degToRad(angle)`, `radToDeg(angle)`

---

### 5.5 helper.h

**Includes:** GLAD, GLFW, GLM (header-only), STB image, filesystem, standard library headers.

**Global GLint locations:** `viewLoc`, `projLoc`, `modelLoc`, `viewLocSB`, `projLocSB` — used as shader uniform locations. Declared here (not ideal, but works for single-file scope).

**`helper` namespace functions:**

| Function | Description |
|----------|-------------|
| `radians(float degrees)` | Degrees to radians (uses `M_PI`) |
| `toRadians(float degrees)` | Same, alternative name |
| `clamp(float value, min, max)` | Clamp value to range |
| `mix(float a, float b, float t)` | Linear interpolation |
| `dotProduct(const vec3&, const vec3&)` | Dot product |
| `crossProduct(const vec3&, const vec3&)` | Cross product |
| `length(const vec3&)` | Vector length (sqrt of dot) |
| `normalize(const vec3&)` | Normalize (returns zero vector if length is 0) |
| `rotAxisAngle(const vec3& axis, float radians)` | Axis-angle rotation matrix (Rodrigues) |
| `getRotationMatrix(char face, float angle)` | Returns rotation matrix for Rubik face chars (U,D,L,R,F,B,V,H,S) |

**Global functions:**

| Function | Description |
|----------|-------------|
| `loadTexture(const char* path)` | Loads image via STB, uploads to GL_TEXTURE_2D, generates mipmaps. Returns texture ID. Verbose error reporting. |
| `checkTextureFiles()` | Verifies hardcoded texture files exist |
| `snapToGrid(vec3&, float gridSize)` | Rounds vertex to nearest grid multiple |
| `normalizeAngle(float angle)` | Wraps angle to [-180, 180] |
| `getCubeCenter(const std::vector<vec3>&)` | Computes centroid of vertices |

---

### 5.6 figura.h

**Abstract base class:** `Figura`

| Member | Type | Purpose |
|--------|------|---------|
| `name` | `std::string` | Identifier |
| `verticesLocales` | `std::vector<vec3>` | Master copy of vertices (immutable reference) |
| `vertices` | `std::vector<vec3>` | Current (transformed) vertices |
| `verticesOrig` | `std::vector<vec3>` | Original positions for reset |
| `vertexColors` | `std::vector<vec3>` | Per-vertex RGB colors |
| `vertexTexCoords` | `std::vector<vec2>` | Per-vertex UVs |
| `VAOs` / `VBOs` / `EBOs` | `std::vector<unsigned int>` | OpenGL buffer objects |
| `indices` | `std::vector<unsigned int>` | Element indices |
| `defaultColors` | `std::vector<vec3>` | Rubik face colors (W,G,R,B,O,Y) |

**Virtual methods:** `updateBuffers()`, `setupBuffers()`, `draw()`, `resetFig()`, `applyTransform()`

**Concrete class:** `Cubo` (inherits `Figura`)

- Builds a cube with 6 faces, each face has 6 vertices (2 triangles)
- Each face can be independently enabled/disabled via `activeFaces[6]`
- Constructor: `Cubo(name, size, position, activeFaces)`
- `buildRect()` — creates 2 triangles from 4 corner points
- `setupBuffers()` — interleaved layout: 3 pos + 3 color + 2 texCoord = 8 floats/vertex
- `draw()` — draws each face as separate `glDrawArrays(GL_TRIANGLES, face*6, 6)`
- `updateBuffers()` — re-uploads vertex data via `glBufferSubData`

---

### 5.7 3dsloader.h / .cpp

**Struct:** `Mesh3DS`

| Member | Type | Description |
|--------|------|-------------|
| `name` | `std::string` | Object name from 3DS file |
| `vertices` | `std::vector<vec3>` | Vertex positions |
| `indices` | `std::vector<unsigned short>` | Triangle indices |
| `texCoords` | `std::vector<vec2>` | UV texture coordinates |

**Function:** `bool Load3DS(Mesh3DS& mesh, const char* filename)`

- Opens 3DS file in binary mode
- Parses chunk-based format (chunk ID + length)
- Handles chunks: `0x4d4d` (MAIN), `0x3d3d` (EDIT), `0x4000` (OBJECT name), `0x4100` (TRIMESH), `0x4110` (VERTEX list), `0x4120` (FACE list), `0x4140` (TEXCOORDS)
- Skips unknown chunks by seeking forward
- Verbose output: vertex count, polygon count, load confirmation

---

### 5.8 camera.h

**Class:** `Camera`

Two modes of operation:

#### Mode A: Orbital Camera (default)
- Position computed from spherical coordinates: `distance`, `pitch`, `yaw`
- Default: `distance=15`, `pitch=35.264` (isometric), `yaw=45`, `fov=45`
- `getViewMatrix()` — computes LookAt from spherical coordinates
- `getPerspectiveMatrix(aspectRatio)` — perspective projection

#### Mode B: Follow Camera (chases the ship)
- Activated via `toggleFollowMode()`
- Position calculated behind ship using ship's yaw/pitch
- Uses exponential smoothing: `smoothedEyePos += (desired - current) * (1 - exp(-smoothing * dt))`
- Offset: `followDistance=8.0f` behind, `followHeight=3.0f` above
- `updateFollow(shipPos, shipYaw, shipPitch, dt)` — must be called every frame
- `getFollowViewMatrix()` — returns cached smoothed view matrix

**Movement methods (orbital mode):**

| Method | Description |
|--------|-------------|
| `orbitUp/Down(dt)` | Changes pitch |
| `orbitLeft/Right(dt)` | Changes yaw |
| `zoomIn/Out(dt)` | Changes distance |
| `translateLeft/Right/Up/Down(dt)` | Pans target point |
| `moveForward/Backward/Left/Right(dt)` | Aliases for orbit functions |

**Camera Animation system:**
- `startCameraAnimation(cubeDuration)` — zooms out and rotates around cube
- `updateCameraAnimation(dt)` — handles ZOOM_OUT and ROTATING states
- Uses ease-out interpolation for zoom

**Hard limits:**

| Parameter | Min | Max |
|-----------|-----|-----|
| Distance | 3.0f | 40.0f |
| FOV | 1.0f | 90.0f |
| Pitch | -89.9f | 89.9f |

**Internal helpers:**
- `clampPitchWithWarning()` — clamps pitch with console warning
- `clampDistanceWithWarning()` — clamps distance
- `normalizeYaw()` — wraps yaw to [0, 360)

---

### 5.9 spaceship.h

**Class:** `Spaceship`

| Member | Type | Description |
|--------|------|-------------|
| `position` | `vec3` | World position (default: 0,0,0) |
| `scale` | `float` | Auto-computed from model size (default: 0.25) |
| `yaw` | `float` | Horizontal rotation in degrees (default: 0) |
| `pitch` | `float` | Vertical rotation in degrees (default: 0) |
| `VAO/VBO/EBO` | `unsigned int` | OpenGL buffers |
| `indexCount` | `int` | Number of indices to draw |
| `m_loaded` | `bool` | Load success flag |

**Methods:**

| Method | Description |
|--------|-------------|
| `load(filepath)` | Loads 3DS, auto-scales to unit size, builds VAO with interleaved data (3 pos + 3 color + 2 UV) |
| `draw(shader, view, proj)` | Uploads model/view/proj matrices, binds VAO, draws elements |
| `setPosition(vec3)` / `getPosition()` | Position getter/setter |
| `moveForward(step)` | Moves along forward vector derived from yaw+pitch |
| `moveBackward(step)` | Moves opposite to forward vector |
| `getForward()` | Computes forward direction from yaw/pitch |
| `getRight()` | Computes right direction (horizontal plane only) |

**Model matrix construction (`getModelMatrix()`):**
1. Build base matrix: `T(position) * Ry(yaw) * scale`
2. Multiply by `Rx(pitch)` for nose up/down
3. Multiply by model correction matrix: `Rz(180) * Rx(90)` — transforms the 3DS model's -Y (cockpit) to -Z (forward in OpenGL)

**3DS model correction:**
```
modelCorr = [-1  0  0  0
              0  0  1  0
              0  1  0  0
              0  0  0  1]
```
This maps: model -Y -> world -Z (forward), model +Z -> world +Y (up).

---

### 5.10 blackhole.h

**Class:** `BlackHole` — self-contained with its own shaders

**Public parameters:**

| Parameter | Default | Description |
|-----------|---------|-------------|
| `position[3]` | (0,0,0) | World position |
| `bhRadius` | 1.5f | Event horizon radius |
| `diskInner` | 2.0f | Inner accretion disk radius |
| `diskOuter` | 5.0f | Outer accretion disk radius |
| `diskParticles` | 200 | Number of disk billboard particles |
| `diskSpeed` | 0.3f | Angular rotation speed |
| `diskAlpha` | 0.7f | Disk transparency |
| `jetLength` | 8.0f | Polar jet length |
| `gasParticleCount` | 150 | Falling gas particles |

**Subsystems:**

1. **Skybox** (`drawSkybox`) — Nebula cube map rendered behind everything. Removes translation from view matrix so camera movement doesn't affect it. Uses `GL_LEQUAL` depth test.

2. **Event Horizon** (`drawSphere`) — Pure black sphere. Uses generated UV sphere mesh (36 sectors, 18 stacks). Shader outputs `vec4(0,0,0,1)`.

3. **Accretion Disk** (`drawDisk`) — Billboard quads arranged in rings. Each particle has:
   - Position computed from `radius = diskInner + t*(diskOuter-diskInner)`
   - Angle: `diskAngle * angularVelocity / diskSpeed`
   - Relativistic beaming: `beaming = 1.0 + 0.8 * cos(angle)` (Doppler effect simulation)
   - Color gradient: white core -> yellow inner -> orange mid -> red outer
   - Emission glow using `pow()` falloff

4. **Photon Ring** (`drawPhotonRing`) — Thin white line loop at `radius = bhRadius * 1.02`. Drawn with `GL_LINE_LOOP`, 256 segments.

5. **Polar Jets** (`drawJets`) — Vertical billboard quads emanating from poles. Segmented along length with fade at tips. Color: white-blue base to blue-violet tip. Multiple angular layers for volume.

6. **Gas Particles** (`drawGasParticles`) — Small quads falling toward the BH. Each has angle, radius, height, speed, heat, size, life. Respawned when life <= 0 or radius < threshold. Currently **not called in draw()** (method exists but is unused in the render pipeline).

**Private members:**
- `progSphere`, `progDisk`, `progJet`, `progParticle`, `progSkybox` — separate shader programs
- `sphereVAO/VBO/EBO`, `quadVAO`, `skyboxVAO/VBO/Tex`
- `diskAngle` — accumulated rotation angle
- `gasParticles` — vector of `GasParticle` structs

**Shader Programs (all inline const char*):**

| Shader | Type | Key feature |
|--------|------|-------------|
| `BH_VERT_SRC` | Vertex | Standard transform, passes color+UV |
| `BH_SPHERE_FRAG_SRC` | Fragment | Pure black output |
| `BH_DISK_FRAG_SRC` | Fragment | Radial gradient + beaming + emission |
| `BH_JET_FRAG_SRC` | Fragment | Vertical fade + glow + progress-based color |
| `BH_PARTICLE_FRAG_SRC` | Fragment | Circular fade + heat-based color |
| `BH_SKYBOX_VERT_SRC` | Vertex | Cube map sampling, `gl_Position = pos.xyww` |
| `BH_SKYBOX_FRAG_SRC` | Fragment | Simple `textureCube` lookup |

---

### 5.11 asteroid.h

**Class:** `Asteroid`

**Critical design:** Uses **static shared mesh** — all asteroid instances share a single VAO/VBO/EBO to minimize GPU memory. The mesh is loaded once via `Asteroid::loadMesh(filepath)`.

| Member | Type | Description |
|--------|------|-------------|
| `position` | `vec3` | Current world position |
| `direction` | `vec3` | Normalized movement direction |
| `speed` | `float` | Movement speed (4.5f) |
| `scale` | `float` | Size multiplier |
| `rotX/Y/Z` | `float` | Current rotation angles |
| `rotSpeedX/Y/Z` | `float` | Rotation speeds (random) |

**Static members:** `sharedVAO`, `sharedVBO`, `sharedEBO`, `sharedIndexCount`, `isMeshLoaded`

**Methods:**

| Method | Description |
|--------|-------------|
| `loadMesh(filepath)` | Static. Loads 3DS asteroid, applies spherical UV fallback, builds shared VAO. Call once at init. |
| `Asteroid(startPos, targetPos, sizeFactor)` | Constructor. Computes direction vector toward target. Random rotation speeds. |
| `update(dt)` | Moves along direction, applies rotation |
| `draw(shader)` | Uploads model matrix, binds shared VAO, draws |
| `getModelMatrix()` | Builds TRS matrix with rotation + corrective scale (0.001f) |

**Spherical UV fallback:** If the 3DS model lacks texture coordinates, computes UVs from vertex direction:
```
u = 0.5 + atan2(nz, nx) / (2*pi)
v = 0.5 + asin(ny) / pi
```

**Model matrix layout:** Row-major with combined rotation (X then Y) and scale.

---

### 5.12 skybox.h

**Class:** `SkyBox`

Generic cube map skybox renderer. Currently **not used in main.cpp** — the black hole has its own integrated skybox.

| Method | Description |
|--------|-------------|
| `init(faces)` | Loads 6 cubemap faces via STB, builds VAO with 36 vertices |
| `draw(view, projection)` | Renders with `GL_LEQUAL` depth. Removes translation from view matrix (so skybox stays at infinity). |

**Vertex shader:** `TexCoords = aPos`, `gl_Position = projection * mat3(view) * vec4(aPos, 1.0)`, then `gl_Position = pos.xyww` (forces w=1 for infinite depth).

**Fragment shader:** Simple `textureCube` lookup.

**Keep for:** Potential reuse if the black hole skybox needs to be separated, or for alternative environments.

---

## 6. Shader System

### Main Shader (used by Rubik, Spaceship, Asteroids)

**Vertex attributes:**
- Location 0: `vec3 aPos` — vertex position
- Location 1: `vec3 aColor` — per-vertex color
- Location 2: `vec2 aTexCoord` — texture coordinates

**Uniforms:**
- `mat4 model` — per-object transform
- `mat4 view` — camera view matrix
- `mat4 projection` — perspective projection

**Fragment output:** Mix of texture and vertex color based on alpha threshold.

### Black Hole Shaders (self-contained)

The black hole compiles 5 separate shader programs at `init()` time. Each has its own vertex and fragment shader. They share the same vertex attribute layout (location 0,1,2) as the main shader.

### Current Limitations

- **No normal vectors** are computed, passed, or used
- **No lighting model** (ambient, diffuse, specular)
- The fragment shader is purely texture * color blending
- Material properties don't exist
- The black hole glow is faked via emission in the fragment shader, not actual light sources

---

## 7. Input & Controls

### Keyboard (discrete — key_callback)

| Key | Action |
|-----|--------|
| `ESC` | Close window |
| `TAB` | Toggle Rubik rotation direction |
| `T` | Rotate U face |
| `R` | Rotate L face |
| `F` (Rubik) | Rotate F face |
| `G` | Rotate R face |
| `Y` | Rotate B face |
| `H` | Rotate D face |
| `V` | Rotate vertical slice |
| `B` | Rotate horizontal slice |
| `N` | Rotate S slice |
| `K` | Reset Rubik cube |
| `I` | Wireframe mode |
| `O` | Solid fill mode |
| `0` | Point mode |
| `P` | Print menu |
| `J` | Solve Rubik |
| `M` | Scramble Rubik (50 moves) |
| `1-5` | Sequence speed: x1, x2, x4, x8, x16 |
| `=` / `-` | Increase/decrease speed |
| `L` | Random background color |
| `F` (toggle) | **Toggle camera follow mode** |

### Keyboard (continuous — processInput)

| Key | Action (Orbital) | Action (Follow) |
|-----|-----------------|-----------------|
| `W` | Orbit up (pitch+) | — |
| `S` | Orbit down (pitch-) | — |
| `A` | Orbit left (yaw-) | — |
| `D` | Orbit right (yaw+) | — |
| `Q` | Zoom in | — |
| `E` | Zoom out | — |
| `UP` | — | Ship forward |
| `DOWN` | — | Ship backward |
| `LEFT` | — | Ship yaw left |
| `RIGHT` | — | Ship yaw right |
| `Z` | — | Rotate Rubik global X+ |
| `X` | — | Rotate Rubik global X- |
| `C` | — | Rotate Rubik global Y+ |

### Mouse

- `cursor_position_callback` — controls spaceship look direction
- Cursor is captured and hidden (`GLFW_CURSOR_DISABLED`)
- Yaw: horizontal mouse movement (inverted)
- Pitch: vertical mouse movement
- Clamped to [-89, 89] degrees to prevent gimbal lock
- `mouseSensitivity = 0.1f`

---

## 8. Rendering Pipeline

### Per-frame order (main.cpp render loop):

```
1. Clear (COLOR | DEPTH)
2. BlackHole::update(dt)
3. BlackHole::draw(view, proj)          -- own shaders
   3a. drawSkybox()                      -- GL_LEQUAL, no depth write
   3b. drawSphere()                      -- pure black, depth test ON
   3c. drawDisk()                        -- blending ON, depth write OFF
   3d. drawPhotonRing()                  -- white line loop
   3e. drawJets()                        -- blending ON, depth write OFF

4. glUseProgram(mainShader)
5. Upload view + projection matrices

6. Rubik's Cube
   glBindTexture(cubitoBorder.png)
   cuboRubik->draw(shader)

7. Asteroid spawning logic
   if (time since last > 1.5s):
       spawn at random XY, Z=-50, targeting ship

8. Asteroids
   glBindTexture(asteroide.jpg)
   for each asteroid:
       asteroid.update(dt)
       asteroid.draw(shader)
       remove if asteroid.position.z > 2

9. Spaceship
   glBindTexture(spaceshiptexture.bmp)
   spaceship.draw(shader, view, proj)

10. SwapBuffers + PollEvents
```

### Depth/Blend state management:

- `GL_DEPTH_TEST` enabled globally
- Black hole: enables/disables blending per subsystem
- Accretion disk + jets: `glDepthMask(GL_FALSE)` to avoid z-fighting with transparent quads
- Skybox: `glDepthMask(GL_FALSE)` + `GL_LEQUAL`

---

## 9. Asset Inventory

| Asset File | Format | Used By | Purpose |
|------------|--------|---------|---------|
| `spaceship.3DS` | 3DS | `Spaceship::load()` | Player ship geometry |
| `spaceshiptexture.bmp` | BMP | main render loop | Ship diffuse map |
| `asteroide.3ds` | 3DS | `Asteroid::loadMesh()` | Asteroid geometry (shared) |
| `asteroide.jpg` | JPEG | main render loop | Asteroid diffuse map |
| `cubitoBorder.png` | PNG | main render loop | Rubik cube face texture |
| `bh_nebula/*.png` | PNG | `BlackHole::initSkybox()` | Nebula cube map (6 faces) |
| `bh_colormap.png` | PNG | Currently unused | Potential BH color reference |

---

## 10. Global State

### Critical globals (main.cpp scope):

```cpp
std::vector<Asteroid> listaAsteroides;     // All active asteroids
float tiempoUltimoAsteroide;               // Last spawn timestamp
float frecuenciaSpawn;                     // Spawn interval
unsigned int asteroideTexID;               // Asteroid texture

Camera camera;                             // Global camera
CuboRubik* cuboRubik;                      // Rubik instance
Spaceship spaceship;                       // Ship instance
bool isClockwise;                          // Rubik dir

float lastFrame, deltaTime, currentFrame;  // Timing
float lastMouseX, lastMouseY;              // Mouse state
bool firstMouse;

colorVec backgroundColor;                  // Clear color
GLenum currentDrawMode;                    // Polygon mode
```

### Global shader locations (helper.h):

```cpp
GLint viewLoc, projLoc, modelLoc;          // Main shader
GLint viewLocSB, projLocSB;                // Skybox shader
```

### Asteroid statics (asteroid.h):

```cpp
unsigned int Asteroid::sharedVAO = 0;
unsigned int Asteroid::sharedVBO = 0;
unsigned int Asteroid::sharedEBO = 0;
int Asteroid::sharedIndexCount = 0;
bool Asteroid::isMeshLoaded = false;
```

---

## Quick Reference: Adding a New Rendered Object

To add a new 3D object to the scene:

1. **Create a header file** (e.g., `new_object.h`) with:
   - Position, rotation, scale members
   - VAO/VBO/EBO handles
   - `load()` method to build geometry
   - `draw(shaderProgram)` method to render
   - `update(dt)` for animation

2. **In `main.cpp`:**
   - Include the header
   - Declare a global instance
   - Call `load()` during initialization
   - Call `update(dt)` and `draw(shader)` in the render loop
   - Bind appropriate texture before drawing

3. **Vertex format must match:** 3 floats position + 3 floats color + 2 floats UV = 8 floats/vertex

4. **Model matrix:** Build a `matriz4x4`, upload via `glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data())`

5. **If the object needs lighting** (see lighting plan document), you'll need to extend the vertex format to include normals (3 floats) and update the shader.
