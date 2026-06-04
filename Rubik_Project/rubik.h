#ifndef RUBIK_H_
#define RUBIK_H_

#include "figura.h"
#include "helper.h"
#include "camera.h"

#include <iostream>
#include <cstdint>
#include <vector>
using std::vector;
#include <map>
using std::map;
#include <array>
using std::array;
#include <memory>
using std::unique_ptr;
using std::make_unique;
using std::string;


class CuboRubik
{
private:
    typedef unique_ptr<Cubo> cubePtr;

    map<string, cubePtr>            cubeMap;
    map<string, array<string, 9>>   faceMap;
    map<string, array<string, 8>>   sliceMap;

    Camera * camera;

    // cristian
	float acumTx = 0.0f, acumTy = 0.0f, acumTz = 0.0f;
	float acumRotX = 0.0f, acumRotY = 0.0f;

    float lastFrameTime;
    // rotation affected cubes

    void initializeCubes();
    // Helper method to initialize cube buffers
    void setupBuffers();
    // Helper method to update cube buffer data
    void updateCubeBuffer(const std::string& cubeName);
    
    /**
     * FACE AND SLICE ROTATION ANIMATION
     */
    // rotation affected cubes
    std::vector<std::string> affectedCubes;
    void rotateFace(char face, float angle);
    void updateCubePositions(const std::vector<string>& affectedCubes, char face, float angle);
    void updateFaceMapAfterRotation(char face, bool clockwise);
	void updateSliceMapAfterRotation(char face, bool clockwise);
    void updateAdjacentFaces(char face, bool clockwise);

    // ---- Animation System (Smooth Rotation) ----
    /**
     * Holds the complete state for an in-progress face or slice rotation animation.
     * Instead of applying the full rotation instantly, we animate it over multiple
     * frames by interpolating the angle from 0 to target_angle.
     * 
     * To avoid floating-point drift, each frame restores the affected cubes' vertices
     * from saved starting snapshots and applies the total rotation for the current
     * progress. On the final frame, we apply the exact target angle mathematically.
     */
    class Face_Rotation_Animation {
		public:
        bool is_running = false;           // Whether an animation is currently active
        char face = '\0';                  // Which face/slice is rotating (e.g., 'U', 'L', 'V')
        std::vector<std::string> affected_cube_names;  // Names of cubes being rotated
        std::vector<std::vector<vec3>> affected_cubes_starting_vertices;  // Snapshot of vertices at animation start
        float target_angle = 0.0f;         // Final angle to reach (e.g., +90.0f or -90.0f)
        float elapsed_time = 0.0f;         // Accumulated time since animation started
        float duration_seconds = 0.25f;    // How long the animation should last
        bool is_clockwise = true;          // Direction of rotation
        bool is_slice_rotation = false;    // Whether this is a slice (middle-layer) rotation
    };
    Face_Rotation_Animation current_animation;

    /**
     * Computes the rotation matrix for a given face/slice and angle.
     * Uses the same convention as updateCubePositions to ensure consistency.
     * 
     * For faces on the negative side of their axis (L, D, B), we invert the angle
     * so the physical matrix rotation matches the logical clockwise/counter-clockwise
     * direction.
     */
    matriz4x4 compute_rotation_matrix_for_face(char face, float angle) {
        Transform t;
        float phys_angle = angle;
        // Faces on the negative side of their axis need angle inversion
        // to match the logical clockwise/counter-clockwise mapping.
        if (face == 'L' || face == 'D' || face == 'B') {
            phys_angle = -angle; 
        }
        switch(face) {
            case 'R': 
            case 'L':
			case 'V':
                return t.rotacionX(phys_angle);
            case 'U': 
            case 'D':
			case 'H':		
                return t.rotacionY(phys_angle);
            case 'F': 
            case 'B':
			case 'S':
                return t.rotacionZ(phys_angle);
        }
        // This should never happen if face is valid.
        std::cerr << "[ERROR][compute_rotation_matrix_for_face] Unknown face '" 
                  << face << "', returning identity matrix." << std::endl;
        return t.rotacionY(0.0f);  // Identity (no rotation)
    }

    /**
     * Applies a rotation of the given angle to all affected cubes in the
     * current animation, restoring from saved starting vertices first.
     * 
     * This is the core of the drift-free approach: we always rotate from the
     * original snapshot, never from the previous frame's result.
     */
    void apply_rotation_to_affected_cubes(float angle) {
        matriz4x4 rotation = compute_rotation_matrix_for_face(current_animation.face, angle);
        
        for (size_t i = 0; i < current_animation.affected_cube_names.size(); i++) {
            const std::string& cube_name = current_animation.affected_cube_names[i];
            auto it = cubeMap.find(cube_name);
            if (it == cubeMap.end() || !it->second) {
                std::cerr << "[WARNING][apply_rotation_to_affected_cubes] Cube '" 
                          << cube_name << "' not found or is null, skipping." << std::endl;
                continue;
            }
            
            // 1. Restore vertices from the saved snapshot (drift-free)
            it->second->vertices = current_animation.affected_cubes_starting_vertices[i];
            
            // 2. Apply the rotation matrix to the restored vertices
            it->second->vertices = rotation.multFig(it->second->vertices);
            
            // 3. Push updated vertices to the GPU
            it->second->updateBuffers();
        }
    }

    /**
     * Finalizes the animation by applying the exact target rotation and
     * updating the logical face/slice maps. This ensures the mathematical
     * precision of the final state (no floating-point accumulation errors).
     */
    void finalize_animation() {
        if (!current_animation.is_running) {
            std::cerr << "[WARNING][finalize_animation] Called but no animation is running." << std::endl;
            return;
        }

        std::cout << "[ANIMATION] Finalizing rotation of face '" << current_animation.face 
                  << "' to exact angle " << current_animation.target_angle << " degrees." << std::endl;

        // Apply the mathematically exact final rotation from saved starting vertices
        apply_rotation_to_affected_cubes(current_animation.target_angle);

        // Now update the logical maps to reflect the completed rotation
        if (current_animation.is_slice_rotation) {
            updateSliceMapAfterRotation(current_animation.face, current_animation.is_clockwise);
        } else {
            updateFaceMapAfterRotation(current_animation.face, current_animation.is_clockwise);
        }

        // Clear animation state
        current_animation.is_running = false;
        current_animation.affected_cube_names.clear();
        current_animation.affected_cubes_starting_vertices.clear();
        
        std::cout << "[ANIMATION] Rotation complete. Ready for next input." << std::endl;
    }

public:
    CuboRubik(float lastFrameTime, Camera & cam);
    void init(){
        initializeCubes();
    }

    void resetRubik();

    void draw(unsigned int shaderProgram);

    void rotateU(bool clockwise = true);
    void rotateD(bool clockwise = true);
    void rotateL(bool clockwise = true);
    void rotateR(bool clockwise = true);
    void rotateF(bool clockwise = true);
    void rotateB(bool clockwise = true);
	void rotateSV(bool clockwise = true);
	void rotateSH(bool clockwise = true);
	void rotateSS(bool clockwise = true);

    void applyTransform(matriz4x4 m);

    // Helper method to debug face maps
    void printFaceMap(char face);
    // Helper method to debug slice maps
    void printSliceMap(char slice);

    // cristian
    void irAlOrigen();
	void regresarAPosicionGlobal();
	void actualizarMatrizMundo();
    //ROTACIONESS
	void trasladarCuboGlobal(float tx, float ty, float tz);
	void rotarCuboGlobalX(float angulo);
	void rotarCuboGlobalY(float angulo);

    // ---- Animation System Public Interface ----
    /**
     * Advances the face/slice rotation animation by delta_time seconds.
     * Call this once per frame from the main render loop.
     * 
     * The animation works by:
     * 1. Restoring affected cubes' vertices from a saved snapshot each frame
     * 2. Applying the total rotation for the current progress (0% to 100%)
     * 3. On the final frame, applying the exact mathematically-correct angle
     *    and updating the logical face/slice maps.
     */
    void update_animation(float delta_time) {
        if (!current_animation.is_running) {
            return;  // Nothing to animate
        }

        // Guard against invalid duration to prevent division by zero
        if (current_animation.duration_seconds <= 0.0f) {
            std::cerr << "[ERROR][update_animation] Invalid duration (" 
                      << current_animation.duration_seconds 
                      << "s), forcing immediate completion." << std::endl;
            finalize_animation();
            return;
        }

        // Advance time
        current_animation.elapsed_time += delta_time;
        
        // Calculate progress as a value from 0.0 to 1.0
        float progress = current_animation.elapsed_time / current_animation.duration_seconds;
        
        // Check if animation has reached or exceeded its target duration
        if (progress >= 1.0f) {
            // Final frame: snap to the exact mathematically-correct final position
            finalize_animation();
            return;
        }

        // Interpolate the angle for this frame based on progress
        // Example: if target is -90 and progress is 0.5, current angle is -45
        float current_angle = current_animation.target_angle * progress;
        
        // Apply rotation from saved starting vertices (drift-free)
        apply_rotation_to_affected_cubes(current_angle);
    }

    /**
     * Returns true if a face/slice rotation animation is currently in progress.
     * Use this to block new rotation inputs during animation.
     */
    bool is_animation_running() const {
        return current_animation.is_running;
    }

    ~CuboRubik();
};

void CuboRubik::actualizarMatrizMundo() {
    Transform t;
    matriz4x4 mTras = t.traslacion(vec3(acumTx, acumTy, acumTz));
    matriz4x4 mRotX = t.rotacionX(acumRotX);
    matriz4x4 mRotY = t.rotacionY(acumRotY);

    // Recorremos todos los cubitos
    for (auto& pair : cubeMap) {
        if (pair.second) {
            // 1. Tomamos los v�rtices limpios del origen (copia maestra)
            std::vector<vec3> verticesLimpios = pair.second->getVerticesLocales(); 
            
            // 2. Aplicamos la transformaci�n en un ORDEN ESTRICTO que jam�s cambia
            verticesLimpios = mRotY.multFig(verticesLimpios);
            verticesLimpios = mRotX.multFig(verticesLimpios);
            verticesLimpios = mTras.multFig(verticesLimpios);
            
            // 3. Guardamos el resultado en los v�rtices de renderizado y subimos a la GPU
            pair.second->setVertices(verticesLimpios);
            pair.second->updateBuffers();
        }
    }
}

void CuboRubik::irAlOrigen() {
    Transform t;
    // Aplicamos lo opuesto en orden inverso
    matriz4x4 mRotY = t.rotacionY(-acumRotY);
    matriz4x4 mRotX = t.rotacionX(-acumRotX);
    matriz4x4 mTras = t.traslacion(vec3(-acumTx, -acumTy, -acumTz));
    
    for (auto& pair : cubeMap) {
        if (pair.second) {
            pair.second->applyTransform(mTras);
            pair.second->applyTransform(mRotX);
            pair.second->applyTransform(mRotY);
        }
    }
}

void CuboRubik::regresarAPosicionGlobal() {
    Transform t;
    // Volvemos a colocarlo donde pertenece
    matriz4x4 mRotY = t.rotacionY(acumRotY);
    matriz4x4 mRotX = t.rotacionX(acumRotX);
    matriz4x4 mTras = t.traslacion(vec3(acumTx, acumTy, acumTz));
    
    for (auto& pair : cubeMap) {
        if (pair.second) {
            pair.second->applyTransform(mRotY);
            pair.second->applyTransform(mRotX);
            pair.second->applyTransform(mTras);
        }
    }
}


void CuboRubik::initializeCubes() {
        // Create the 26 outer cubes (excluding center)
        //const float offset = 1.02f;  // Slight gap between cubes
        const float offset = 1.1f;  // Slight gap between cubes

        
        vector<array<float, 3>> positions = {
            // Left layer
            {-offset, -offset, -offset}, {-offset, -offset, 0.0f}, {-offset, -offset, offset},
            {-offset, 0.0f, -offset}, {-offset, 0.0f, 0.0f}, {-offset, 0.0f, offset},
            {-offset, offset, -offset}, {-offset, offset, 0.0f}, {-offset, offset, offset},
            
            // Middle layer
            {0.0f, -offset, -offset}, {0.0f, -offset, 0.0f}, {0.0f, -offset, offset},
            {0.0f, 0.0f, -offset}, {0.0f, 0.0f, offset},
            {0.0f, offset, -offset}, {0.0f, offset, 0.0f}, {0.0f, offset, offset},
            
            // Right Layer
            {offset, -offset, -offset}, {offset, -offset, 0.0f}, {offset, -offset, offset},
            {offset, 0.0f, -offset}, {offset, 0.0f, 0.0f}, {offset, 0.0f, offset},
            {offset, offset, -offset}, {offset, offset, 0.0f}, {offset, offset, offset}
        };
        
        vector<string> names = {
            "LDB", "LD", "LDF", "LB", "L", "LF", "LUB", "LU", "LUF",
            "DB", "D", "DF", "B", "F", "UB", "U", "UF",
            "RDB", "RD", "RDF", "RB", "R", "RF", "RUB", "RU", "RUF"
        };

        // 0 = TOP, 1 = LEFT, 2 = FRONT, 3 = RIGHT, 4 = BACK, 5 = BOTTOM
        vector<array<bool, 6>> visibleFaces = {
            // Left layer
            {0, 1, 0, 0, 1, 1}, {0, 1, 0, 0, 0, 1}, {0, 1, 1, 0, 0, 1},
            {0, 1, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0}, {0, 1, 1, 0, 0, 0},
            {1, 1, 0, 0, 1, 0}, {1, 1, 0, 0, 0, 0}, {1, 1, 1, 0, 0, 0},
            
            // Middle layer
            {0, 0, 0, 0, 1, 1}, {0, 0, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 1},
            {0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0},
            {1, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 0, 0}, {1, 0, 1, 0, 0, 0},
            
            // Right Layer
            {0, 0, 0, 1, 1, 1}, {0, 0, 0, 1, 0, 1}, {0, 0, 1, 1, 0, 1},
            {0, 0, 0, 1, 1, 0}, {0, 0, 0, 1, 0, 0}, {0, 0, 1, 1, 0, 0},
            {1, 0, 0, 1, 1, 0}, {1, 0, 0, 1, 0, 0}, {1, 0, 1, 1, 0, 0}
        };
        
        for (size_t i = 0; i < positions.size(); i++) {
            const auto& pos = positions[i];
            cubeMap[names[i]] = make_unique<Cubo>(
                names[i], 1.0f, 
                vec3(pos[0], pos[1], pos[2]),
                visibleFaces[i]
            );
        }

        // Initialize the faces.
        this->faceMap["U"] = {{"LUB", "UB", "RUB", "LU", "U", "RU", "LUF", "UF", "RUF"}};
        this->faceMap["L"] = {{"LUB", "LU", "LUF", "LB", "L", "LF", "LDB", "LD", "LDF"}};
        this->faceMap["F"] = {{"LUF", "UF", "RUF", "LF", "F", "RF", "LDF", "DF", "RDF"}};
        this->faceMap["R"] = {{"RUF", "RU", "RUB", "RF", "R", "RB", "RDF", "RD", "RDB"}};
        this->faceMap["B"] = {{"RUB", "UB", "LUB", "RB", "B", "LB", "RDB", "DB", "LDB"}};
        this->faceMap["D"] = {{"LDF", "DF", "RDF", "LD", "D", "RD", "LDB", "DB", "RDB"}};

        // Initialize the middle slices.
        this->sliceMap["V"] = {{"UB", "U", "UF", "F", "DF", "D", "DB", "B"}};
        this->sliceMap["H"] = {{"LB", "L", "LF", "F", "RF", "R", "RB", "B"}};
        this->sliceMap["S"] = {{"LU", "U", "RU", "R", "RD", "D", "LD", "L"}}; // view from front
    }



void CuboRubik::draw(unsigned int shaderProgram) {
    for (const auto& cubePair : cubeMap) {
        cubePair.second->draw(shaderProgram);
    }
}

CuboRubik::CuboRubik(float lastFrameTime, Camera & cam) : lastFrameTime(lastFrameTime) {
    std::cout << "Rubik's Cube Constructor" << std::endl;
    camera = &cam;
    //initializeCubes();    
    //setupBuffers();
}

/*------------------------------------------------------------------------------------*/
/*----------------------- FACE AND SLICE ROTATION ANIMATION -----------------------*/
/*------------------------------------------------------------------------------------*/

void CuboRubik::rotateFace(char face, float angle) {
    // Block new rotation requests while an animation is already running
    if (current_animation.is_running) {
        std::cout << "[ANIMATION][BLOCKED] Rotation of face '" << face 
                  << "' ignored because another rotation is currently animating." << std::endl;
        return;
    }

    angle = normalizeAngle(angle);
    
    // Determine if this is a face or slice rotation
    bool is_slice = false;
    std::vector<std::string> cubes_to_rotate;
    
    if (faceMap.find(std::string(1, face)) != faceMap.end()) {
        // Face rotation
        const auto& faceCubes = faceMap[std::string(1, face)];
        cubes_to_rotate.insert(cubes_to_rotate.end(), faceCubes.begin(), faceCubes.end());
        std::cout << "[ANIMATION] Starting face rotation: face=" << face 
                  << ", angle=" << angle << ", cubes=" << cubes_to_rotate.size() << std::endl;
    } else if (sliceMap.find(std::string(1, face)) != sliceMap.end()) {
        // Slice rotation
        const auto& sliceCubes = sliceMap[std::string(1, face)];
        cubes_to_rotate.insert(cubes_to_rotate.end(), sliceCubes.begin(), sliceCubes.end());
        is_slice = true;
        std::cout << "[ANIMATION] Starting slice rotation: slice=" << face 
                  << ", angle=" << angle << ", cubes=" << cubes_to_rotate.size() << std::endl;
    } else {
        std::cerr << "[ERROR][rotateFace] Unknown face or slice: '" << face << "'" << std::endl;
        return;
    }

    // Save the current vertices of all affected cubes as our animation snapshot
    std::vector<std::vector<vec3>> starting_vertices;
    starting_vertices.reserve(cubes_to_rotate.size());
    
    for (const std::string& cube_name : cubes_to_rotate) {
        auto it = cubeMap.find(cube_name);
        if (it == cubeMap.end() || !it->second) {
            std::cerr << "[ERROR][rotateFace] Cannot find cube '" << cube_name 
                      << "' for animation, aborting rotation." << std::endl;
            return;
        }
        // Make a copy of the current vertices as our snapshot
        starting_vertices.push_back(it->second->vertices);
    }

    // Populate the animation state
    current_animation.is_running = true;
    current_animation.face = face;
    current_animation.affected_cube_names = std::move(cubes_to_rotate);
    current_animation.affected_cubes_starting_vertices = std::move(starting_vertices);
    current_animation.target_angle = angle;
    current_animation.elapsed_time = 0.0f;
    current_animation.duration_seconds = 0.25f;  // 0.25 seconds for a snappy but visible animation
    current_animation.is_clockwise = (angle < 0);  // Negative angle = clockwise in this convention
    current_animation.is_slice_rotation = is_slice;

    std::cout << "[ANIMATION] Animation queued: face=" << face 
              << ", target_angle=" << angle 
              << ", duration=" << current_animation.duration_seconds << "s" << std::endl;
}

void CuboRubik::updateCubePositions(const std::vector<string>& affectedCubes, char face, float angle) {
    // NOTE: This method is kept for compatibility but is no longer used directly.
    // The animation system in update_animation() and apply_rotation_to_affected_cubes()
    // replaces its functionality with a drift-free approach.
    
    Transform t;
    matriz4x4 rotation;
    
    // Fix the Desync: L, D, and B faces sit on the negative side of their axes.
    // We invert the angle so the physical matrix rotation perfectly matches 
    // the clockwise/counter-clockwise logical array mapping.
    float physAngle = angle;
    if (face == 'L' || face == 'D' || face == 'B') {
        physAngle = -angle; 
    }

    // Generate the rotation matrix using your custom Transform class
    switch(face) {
        case 'R': 
        case 'L':
		case 'V':
            rotation = t.rotacionX(physAngle); 
            break;
        case 'U': 
        case 'D':
		case 'H':		
            rotation = t.rotacionY(physAngle); 
            break;
        case 'F': 
        case 'B':
		case 'S':
            rotation = t.rotacionZ(physAngle); 
            break;
    }
    
    // Apply transformations
    for (const std::string& cubeName : affectedCubes) {
        auto& cube = cubeMap[cubeName];
        cube->applyTransform(rotation); // Applies transformation and pushes to GPU safely
    }
}

void CuboRubik::updateFaceMapAfterRotation(char face, bool clockwise) {
    auto& faceCubes = faceMap[string(1, face)];
    vector<string> newOrder;
    
    // Different rotation patterns based on face and direction
    if (clockwise) {
        // Clockwise rotation pattern: corners rotate 6->8->2->0->6, edges rotate 7->5->1->3->7
        newOrder = {faceCubes[6], faceCubes[3], faceCubes[0],
                    faceCubes[7], faceCubes[4], faceCubes[1],
                    faceCubes[8], faceCubes[5], faceCubes[2]};
    } else {
        // Counter-clockwise rotation pattern: corners rotate 6->0->2->8->6, edges rotate 7->3->1->5->7
        newOrder = {faceCubes[2], faceCubes[5], faceCubes[8],
                    faceCubes[1], faceCubes[4], faceCubes[7],
                    faceCubes[0], faceCubes[3], faceCubes[6]};
    }
    
    // check if there are no duplicates in a face
    for (size_t i = 0; i < newOrder.size(); i++) {
        for (size_t j = i + 1; j < newOrder.size(); j++) {
            if (newOrder[i] == newOrder[j]) {
                std::cerr << "\t Error: duplicate cube in face " << face << std::endl;
                return;
            }
        }
    }
    
    // Update the face map with new positions
    faceCubes = array<string, 9>();
    for (int i = 0; i < 9; i++) {
        faceCubes[i] = newOrder[i];
    }   

    // Update adjacent faces
    updateAdjacentFaces(face, clockwise);
    printFaceMap(face);
}

void CuboRubik::updateSliceMapAfterRotation(char slice, bool clockwise) {
    auto& sliceCubes = sliceMap[string(1, slice)];
    vector<string> newOrder;
    
    // Different rotation patterns based on slice and direction
    if(slice == 'S'){
        if (clockwise) {
        // Clockwise rotation pattern: corners rotate 6->8->2->0->6, edges rotate 7->5->1->3->7
            newOrder = {sliceCubes[6], sliceCubes[7], sliceCubes[0],
                        sliceCubes[1],                 sliceCubes[2],
                        sliceCubes[3], sliceCubes[4], sliceCubes[5]};
        } else {
        // Counter-clockwise rotation pattern: corners rotate 6->0->2->8->6, edges rotate 7->3->1->5->7
            newOrder = {sliceCubes[2], sliceCubes[3], sliceCubes[4],
                        sliceCubes[5],                 sliceCubes[6], 
                        sliceCubes[7], sliceCubes[0], sliceCubes[1]};
        }
    }
        else if(slice == 'V' || slice == 'H'){
        if (clockwise) {
        // Clockwise rotation pattern: corners rotate 6->8->2->0->6, edges rotate 7->5->1->3->7
            newOrder = {sliceCubes[2], sliceCubes[3], sliceCubes[4],
                        sliceCubes[5],                sliceCubes[6],
                        sliceCubes[7], sliceCubes[0], sliceCubes[1]};
        } else {
        // Counter-clockwise rotation pattern: corners rotate 6->0->2->8->6, edges rotate 7->3->1->5->7
            newOrder = {sliceCubes[6], sliceCubes[7], sliceCubes[0],
                        sliceCubes[1],                sliceCubes[2], 
                        sliceCubes[3], sliceCubes[4], sliceCubes[5]};
        }
    }
    
    
    // check if there are no duplicates in a slice
    for (size_t i = 0; i < newOrder.size(); i++) {
        for (size_t j = i + 1; j < newOrder.size(); j++) {
            if (newOrder[i] == newOrder[j]) {
                std::cerr << "\t Error: duplicate cube in slice " << slice << std::endl;
                return;
            }
        }
    }
    
    // Update the slice map with new positions
    sliceCubes = array<string, 8>();
    for (int i = 0; i < 8; i++) {
        sliceCubes[i] = newOrder[i];
    }   

    // Update adjacent faces
    updateAdjacentFaces(slice, clockwise);
}

void CuboRubik::updateAdjacentFaces(char face, bool clockwise) {
        vector<string> temp;
        switch(face) {
            case 'U':
                if (clockwise) {
                    // Move top to right (U -> R)
                    faceMap["R"][2] = faceMap["U"][2];
                    faceMap["R"][1] = faceMap["U"][5];
                    faceMap["R"][0] = faceMap["U"][8];
                    // Move right to bottom (U -> F)
                    faceMap["F"][0] = faceMap["U"][6];
                    faceMap["F"][1] = faceMap["U"][7];
                    faceMap["F"][2] = faceMap["U"][8];
                    // Move bottom to left (U -> L)
                    faceMap["L"][0] = faceMap["U"][0];
                    faceMap["L"][1] = faceMap["U"][3];
                    faceMap["L"][2] = faceMap["U"][6];
                    // Move top to back (U -> B)
                    faceMap["B"][2] = faceMap["U"][0];
                    faceMap["B"][1] = faceMap["U"][1];
                    faceMap["B"][0] = faceMap["U"][2];
                    // side middle slice
                    sliceMap["S"][0] = faceMap["U"][3];
                    sliceMap["S"][1] = faceMap["U"][4];
                    sliceMap["S"][2] = faceMap["U"][5];
                    // middle vertical slice
                    sliceMap["V"][0] = faceMap["U"][1];
                    sliceMap["V"][1] = faceMap["U"][4];
                    sliceMap["V"][2] = faceMap["U"][7];
                } else {
                    faceMap["R"][2] = faceMap["U"][2];
                    faceMap["R"][1] = faceMap["U"][5];
                    faceMap["R"][0] = faceMap["U"][8];
                    
                    faceMap["F"][0] = faceMap["U"][6];
                    faceMap["F"][1] = faceMap["U"][7];
                    faceMap["F"][2] = faceMap["U"][8];
                    
                    faceMap["L"][0] = faceMap["U"][0];
                    faceMap["L"][1] = faceMap["U"][3];
                    faceMap["L"][2] = faceMap["U"][6];
                    
                    faceMap["B"][2] = faceMap["U"][0];
                    faceMap["B"][1] = faceMap["U"][1];
                    faceMap["B"][0] = faceMap["U"][2];

                    sliceMap["S"][0] = faceMap["U"][3];
                    sliceMap["S"][1] = faceMap["U"][4];
                    sliceMap["S"][2] = faceMap["U"][5];
                    sliceMap["V"][0] = faceMap["U"][1];
                    sliceMap["V"][1] = faceMap["U"][4];
                    sliceMap["V"][2] = faceMap["U"][7];
                }
                break;

            case 'L':
                if (clockwise) {
                    faceMap["F"][0] = faceMap["L"][2];
                    faceMap["F"][3] = faceMap["L"][5];
                    faceMap["F"][6] = faceMap["L"][8];

                    faceMap["D"][0] = faceMap["L"][8];
                    faceMap["D"][3] = faceMap["L"][7];
                    faceMap["D"][6] = faceMap["L"][6];

                    faceMap["B"][2] = faceMap["L"][0];
                    faceMap["B"][5] = faceMap["L"][3];
                    faceMap["B"][8] = faceMap["L"][6];

                    faceMap["U"][0] = faceMap["L"][0];
                    faceMap["U"][3] = faceMap["L"][1];
                    faceMap["U"][6] = faceMap["L"][2];

                    sliceMap["S"][0] = faceMap["L"][1];
                    sliceMap["S"][7] = faceMap["L"][4];
                    sliceMap["S"][6] = faceMap["L"][7];
                    sliceMap["H"][0] = faceMap["L"][3];
                    sliceMap["H"][1] = faceMap["L"][4];
                    sliceMap["H"][2] = faceMap["L"][5];
                    
                } else {
                    faceMap["F"][0] = faceMap["L"][2];
                    faceMap["F"][3] = faceMap["L"][5];
                    faceMap["F"][6] = faceMap["L"][8];

                    faceMap["D"][0] = faceMap["L"][8];
                    faceMap["D"][3] = faceMap["L"][7];
                    faceMap["D"][6] = faceMap["L"][6];

                    faceMap["B"][2] = faceMap["L"][0];
                    faceMap["B"][5] = faceMap["L"][3];
                    faceMap["B"][8] = faceMap["L"][6];

                    faceMap["U"][0] = faceMap["L"][0];
                    faceMap["U"][3] = faceMap["L"][1];
                    faceMap["U"][6] = faceMap["L"][2];

                    sliceMap["S"][0] = faceMap["L"][1];
                    sliceMap["S"][7] = faceMap["L"][4];
                    sliceMap["S"][6] = faceMap["L"][7];
                    sliceMap["H"][0] = faceMap["L"][3];
                    sliceMap["H"][1] = faceMap["L"][4];
                    sliceMap["H"][2] = faceMap["L"][5];
                }
                break;

            case 'F':
                if (clockwise) {
                    // Move top to right (F -> R)
                    faceMap["R"][0] = faceMap["F"][2];
                    faceMap["R"][3] = faceMap["F"][5];
                    faceMap["R"][6] = faceMap["F"][8];
                    // Move right to bottom (F -> D)
                    faceMap["D"][0] = faceMap["F"][6];
                    faceMap["D"][1] = faceMap["F"][7];
                    faceMap["D"][2] = faceMap["F"][8];
                    // Move bottom to left (F -> L)
                    faceMap["L"][2] = faceMap["F"][0];
                    faceMap["L"][5] = faceMap["F"][3];
                    faceMap["L"][8] = faceMap["F"][6];
                    // Move left to top (F -> U)
                    faceMap["U"][6] = faceMap["F"][0];
                    faceMap["U"][7] = faceMap["F"][1];
                    faceMap["U"][8] = faceMap["F"][2];

                    sliceMap["V"][2] = faceMap["F"][1];
                    sliceMap["V"][3] = faceMap["F"][4];
                    sliceMap["V"][4] = faceMap["F"][7];
                    sliceMap["H"][2] = faceMap["F"][3];
                    sliceMap["H"][3] = faceMap["F"][4];
                    sliceMap["H"][4] = faceMap["F"][5];
                } else {
                    // Move top to right (F -> R)
                    faceMap["R"][0] = faceMap["F"][2];
                    faceMap["R"][3] = faceMap["F"][5];
                    faceMap["R"][6] = faceMap["F"][8];
                    // Move right to bottom (F -> D)
                    faceMap["D"][0] = faceMap["F"][6];
                    faceMap["D"][1] = faceMap["F"][7];
                    faceMap["D"][2] = faceMap["F"][8];
                    // Move bottom to left (F -> L)
                    faceMap["L"][2] = faceMap["F"][0];
                    faceMap["L"][5] = faceMap["F"][3];
                    faceMap["L"][8] = faceMap["F"][6];
                    // Move left to top (F -> U)
                    faceMap["U"][6] = faceMap["F"][0];
                    faceMap["U"][7] = faceMap["F"][1];
                    faceMap["U"][8] = faceMap["F"][2];

                    sliceMap["V"][2] = faceMap["F"][1];
                    sliceMap["V"][3] = faceMap["F"][4];
                    sliceMap["V"][4] = faceMap["F"][7];
                    sliceMap["H"][2] = faceMap["F"][3];
                    sliceMap["H"][3] = faceMap["F"][4];
                    sliceMap["H"][4] = faceMap["F"][5];
                }
                break;

            case 'R':
                if (clockwise) {
                    faceMap["B"][0] = faceMap["R"][2];
                    faceMap["B"][3] = faceMap["R"][5];
                    faceMap["B"][6] = faceMap["R"][8];

                    faceMap["D"][2] = faceMap["R"][6];
                    faceMap["D"][5] = faceMap["R"][7];
                    faceMap["D"][8] = faceMap["R"][8];

                    faceMap["F"][2] = faceMap["R"][0];
                    faceMap["F"][5] = faceMap["R"][3];
                    faceMap["F"][8] = faceMap["R"][6];

                    faceMap["U"][2] = faceMap["R"][2];
                    faceMap["U"][5] = faceMap["R"][1];
                    faceMap["U"][8] = faceMap["R"][0];

                    sliceMap["S"][2] = faceMap["R"][1];
                    sliceMap["S"][3] = faceMap["R"][4];
                    sliceMap["S"][4] = faceMap["R"][7];

                    sliceMap["H"][6] = faceMap["R"][5];
                    sliceMap["H"][5] = faceMap["R"][4];
                    sliceMap["H"][4] = faceMap["R"][3];
                } else {
                    faceMap["B"][0] = faceMap["R"][2];
                    faceMap["B"][3] = faceMap["R"][5];
                    faceMap["B"][6] = faceMap["R"][8];

                    faceMap["D"][2] = faceMap["R"][6];
                    faceMap["D"][5] = faceMap["R"][7];
                    faceMap["D"][8] = faceMap["R"][8];

                    faceMap["F"][2] = faceMap["R"][0];
                    faceMap["F"][5] = faceMap["R"][3];
                    faceMap["F"][8] = faceMap["R"][6];

                    faceMap["U"][2] = faceMap["R"][2];
                    faceMap["U"][5] = faceMap["R"][1];
                    faceMap["U"][8] = faceMap["R"][0];

                    sliceMap["S"][2] = faceMap["R"][1];
                    sliceMap["S"][3] = faceMap["R"][4];
                    sliceMap["S"][4] = faceMap["R"][7];
                    sliceMap["H"][6] = faceMap["R"][5];
                    sliceMap["H"][5] = faceMap["R"][4];
                    sliceMap["H"][4] = faceMap["R"][3];
                }
                break;

            case 'B':
                if (clockwise) {
                    faceMap["L"][0] = faceMap["B"][2];
                    faceMap["L"][3] = faceMap["B"][5];
                    faceMap["L"][6] = faceMap["B"][8];

                    faceMap["D"][6] = faceMap["B"][8];
                    faceMap["D"][7] = faceMap["B"][7];
                    faceMap["D"][8] = faceMap["B"][6];

                    faceMap["R"][2] = faceMap["B"][0];
                    faceMap["R"][5] = faceMap["B"][3];
                    faceMap["R"][8] = faceMap["B"][6];
                    
                    faceMap["U"][0] = faceMap["B"][2];
                    faceMap["U"][1] = faceMap["B"][1];
                    faceMap["U"][2] = faceMap["B"][0];

                    sliceMap["V"][0] = faceMap["B"][1];
                    sliceMap["V"][7] = faceMap["B"][4];
                    sliceMap["V"][6] = faceMap["B"][7];
                    sliceMap["H"][0] = faceMap["B"][5];
                    sliceMap["H"][7] = faceMap["B"][4];
                    sliceMap["H"][6] = faceMap["B"][3];
                } else {
                    faceMap["L"][0] = faceMap["B"][2];
                    faceMap["L"][3] = faceMap["B"][5];
                    faceMap["L"][6] = faceMap["B"][8];

                    faceMap["D"][6] = faceMap["B"][8];
                    faceMap["D"][7] = faceMap["B"][7];
                    faceMap["D"][8] = faceMap["B"][6];

                    faceMap["R"][2] = faceMap["B"][0];
                    faceMap["R"][5] = faceMap["B"][3];
                    faceMap["R"][8] = faceMap["B"][6];
                    
                    faceMap["U"][0] = faceMap["B"][2];
                    faceMap["U"][1] = faceMap["B"][1];
                    faceMap["U"][2] = faceMap["B"][0];

                    sliceMap["V"][0] = faceMap["B"][1];
                    sliceMap["V"][7] = faceMap["B"][4];
                    sliceMap["V"][6] = faceMap["B"][7];
                    sliceMap["H"][0] = faceMap["B"][5];
                    sliceMap["H"][7] = faceMap["B"][4];
                    sliceMap["H"][6] = faceMap["B"][3];
                }
                break;
            
            case 'D':
                if (clockwise) {
                    faceMap["R"][6] = faceMap["D"][2];
                    faceMap["R"][7] = faceMap["D"][5];
                    faceMap["R"][8] = faceMap["D"][8];

                    faceMap["B"][6] = faceMap["D"][8];
                    faceMap["B"][7] = faceMap["D"][7];
                    faceMap["B"][8] = faceMap["D"][6];

                    faceMap["L"][6] = faceMap["D"][6];
                    faceMap["L"][7] = faceMap["D"][3];
                    faceMap["L"][8] = faceMap["D"][0];

                    faceMap["F"][6] = faceMap["D"][0];
                    faceMap["F"][7] = faceMap["D"][1];
                    faceMap["F"][8] = faceMap["D"][2];

                    sliceMap["S"][6] = faceMap["D"][3];
                    sliceMap["S"][5] = faceMap["D"][4];
                    sliceMap["S"][4] = faceMap["D"][5];
                    sliceMap["V"][4] = faceMap["D"][1];
                    sliceMap["V"][5] = faceMap["D"][4];
                    sliceMap["V"][6] = faceMap["D"][7];
                } else {
                    faceMap["R"][6] = faceMap["D"][2];
                    faceMap["R"][7] = faceMap["D"][5];
                    faceMap["R"][8] = faceMap["D"][8];

                    faceMap["B"][6] = faceMap["D"][8];
                    faceMap["B"][7] = faceMap["D"][7];
                    faceMap["B"][8] = faceMap["D"][6];

                    faceMap["L"][6] = faceMap["D"][6];
                    faceMap["L"][7] = faceMap["D"][3];
                    faceMap["L"][8] = faceMap["D"][0];

                    faceMap["F"][6] = faceMap["D"][0];
                    faceMap["F"][7] = faceMap["D"][1];
                    faceMap["F"][8] = faceMap["D"][2];

                    sliceMap["S"][6] = faceMap["D"][3];
                    sliceMap["S"][5] = faceMap["D"][4];
                    sliceMap["S"][4] = faceMap["D"][5];
                    sliceMap["V"][4] = faceMap["D"][1];
                    sliceMap["V"][5] = faceMap["D"][4];
                    sliceMap["V"][6] = faceMap["D"][7];
                }
                break;

            case 'S':
                if (clockwise) {
                    faceMap["U"][3] = sliceMap["S"][0];
                    faceMap["U"][4] = sliceMap["S"][1];
                    faceMap["U"][5] = sliceMap["S"][2];
                    faceMap["L"][1] = sliceMap["S"][0];
                    faceMap["L"][4] = sliceMap["S"][7];
                    faceMap["L"][7] = sliceMap["S"][6];
                    faceMap["R"][1] = sliceMap["S"][2];
                    faceMap["R"][4] = sliceMap["S"][3];
                    faceMap["R"][7] = sliceMap["S"][4];
                    faceMap["D"][3] = sliceMap["S"][6];
                    faceMap["D"][4] = sliceMap["S"][5];
                    faceMap["D"][5] = sliceMap["S"][4];

                    sliceMap["V"][1] = sliceMap["S"][1];
                    sliceMap["V"][5] = sliceMap["S"][5];
                    sliceMap["H"][1] = sliceMap["S"][7];
                    sliceMap["H"][5] = sliceMap["S"][3];
                    
                } else {
                    faceMap["U"][3] = sliceMap["S"][0];
                    faceMap["U"][4] = sliceMap["S"][1];
                    faceMap["U"][5] = sliceMap["S"][2];
                    faceMap["L"][1] = sliceMap["S"][0];
                    faceMap["L"][4] = sliceMap["S"][7];
                    faceMap["L"][7] = sliceMap["S"][6];
                    faceMap["R"][1] = sliceMap["S"][2];
                    faceMap["R"][4] = sliceMap["S"][3];
                    faceMap["R"][7] = sliceMap["S"][4];
                    faceMap["D"][3] = sliceMap["S"][6];
                    faceMap["D"][4] = sliceMap["S"][5];
                    faceMap["D"][5] = sliceMap["S"][4];

                    sliceMap["V"][1] = sliceMap["S"][1];
                    sliceMap["V"][5] = sliceMap["S"][5];
                    sliceMap["H"][1] = sliceMap["S"][7];
                    sliceMap["H"][5] = sliceMap["S"][3];
                }
                break;

            case 'V':
                if (clockwise) {
                    faceMap["U"][1] = sliceMap["V"][0];
                    faceMap["U"][4] = sliceMap["V"][1];
                    faceMap["U"][7] = sliceMap["V"][2];
                    faceMap["F"][1] = sliceMap["V"][2];
                    faceMap["F"][4] = sliceMap["V"][3];
                    faceMap["F"][7] = sliceMap["V"][4];
                    faceMap["B"][1] = sliceMap["V"][0];
                    faceMap["B"][4] = sliceMap["V"][7];
                    faceMap["B"][7] = sliceMap["V"][6];
                    faceMap["D"][1] = sliceMap["V"][4];
                    faceMap["D"][4] = sliceMap["V"][5];
                    faceMap["D"][7] = sliceMap["V"][6];

                    sliceMap["S"][1] = sliceMap["V"][1];
                    sliceMap["S"][5] = sliceMap["V"][5];
                    sliceMap["H"][7] = sliceMap["V"][7];
                    sliceMap["H"][3] = sliceMap["V"][3];
                    
                } else {
                    faceMap["U"][1] = sliceMap["V"][0];
                    faceMap["U"][4] = sliceMap["V"][1];
                    faceMap["U"][7] = sliceMap["V"][2];
                    faceMap["F"][1] = sliceMap["V"][2];
                    faceMap["F"][4] = sliceMap["V"][3];
                    faceMap["F"][7] = sliceMap["V"][4];
                    faceMap["B"][1] = sliceMap["V"][0];
                    faceMap["B"][4] = sliceMap["V"][7];
                    faceMap["B"][7] = sliceMap["V"][6];
                    faceMap["D"][1] = sliceMap["V"][4];
                    faceMap["D"][4] = sliceMap["V"][5];
                    faceMap["D"][7] = sliceMap["V"][6];

                    sliceMap["S"][1] = sliceMap["V"][1];
                    sliceMap["S"][5] = sliceMap["V"][5];
                    sliceMap["H"][7] = sliceMap["V"][7];
                    sliceMap["H"][3] = sliceMap["V"][3];
                }
                break;

            case 'H':
                if (clockwise) {
                    faceMap["L"][3] = sliceMap["H"][0];
                    faceMap["L"][4] = sliceMap["H"][1];
                    faceMap["L"][5] = sliceMap["H"][2];
                    faceMap["F"][3] = sliceMap["H"][2];
                    faceMap["F"][4] = sliceMap["H"][3];
                    faceMap["F"][5] = sliceMap["H"][4];
                    faceMap["R"][3] = sliceMap["H"][4];
                    faceMap["R"][4] = sliceMap["H"][5];
                    faceMap["R"][5] = sliceMap["H"][6];
                    faceMap["B"][3] = sliceMap["H"][6];
                    faceMap["B"][4] = sliceMap["H"][7];
                    faceMap["B"][5] = sliceMap["H"][0];

                    sliceMap["S"][7] = sliceMap["H"][1];
                    sliceMap["S"][3] = sliceMap["H"][5];
                    sliceMap["V"][7] = sliceMap["H"][7];
                    sliceMap["V"][3] = sliceMap["H"][3];
                } else {
                    faceMap["L"][3] = sliceMap["H"][0];
                    faceMap["L"][4] = sliceMap["H"][1];
                    faceMap["L"][5] = sliceMap["H"][2];
                    faceMap["F"][3] = sliceMap["H"][2];
                    faceMap["F"][4] = sliceMap["H"][3];
                    faceMap["F"][5] = sliceMap["H"][4];
                    faceMap["R"][3] = sliceMap["H"][4];
                    faceMap["R"][4] = sliceMap["H"][5];
                    faceMap["R"][5] = sliceMap["H"][6];
                    faceMap["B"][3] = sliceMap["H"][6];
                    faceMap["B"][4] = sliceMap["H"][7];
                    faceMap["B"][5] = sliceMap["H"][0];

                    sliceMap["S"][7] = sliceMap["H"][1];
                    sliceMap["S"][3] = sliceMap["H"][5];
                    sliceMap["V"][7] = sliceMap["H"][7];
                    sliceMap["V"][3] = sliceMap["H"][3];
                }
                break;
        }
    }


/*------------------------------------------------------------------------------------*/

void CuboRubik::rotateU(bool clockwise) {
    rotateFace('U', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateL(bool clockwise) {
    rotateFace('L', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateF(bool clockwise) {
    rotateFace('F', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateR(bool clockwise) {
    rotateFace('R', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateB(bool clockwise) {
    rotateFace('B', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateD(bool clockwise) {
    rotateFace('D', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateSV(bool clockwise) {
    rotateFace('V', clockwise ? -90.0f : 90.0f);
}

void CuboRubik::rotateSH(bool clockwise) {
    rotateFace('H', clockwise ? -90.0f : 90.0f);
}
void CuboRubik::rotateSS(bool clockwise) {
    rotateFace('S', clockwise ? -90.0f : 90.0f);
}


void CuboRubik::resetRubik() {
    // Cancel any active animation to prevent stale data access after reset
    if (current_animation.is_running) {
        std::cout << "[ANIMATION] Cancelling active animation due to cube reset." << std::endl;
        current_animation.is_running = false;
        current_animation.affected_cube_names.clear();
        current_animation.affected_cubes_starting_vertices.clear();
        current_animation.elapsed_time = 0.0f;
    }

    // empty cubeMap
    cubeMap.clear();
    // empty faceMap
    faceMap.clear();
    // empty sliceMap
    sliceMap.clear();
    // empty cubeBuffers
    //cubeBuffers.clear();
    // initialize cubes
    initializeCubes();
    // setup buffers
    //setupBuffers();
    std::cout << "Rubik's Cube Restored." << std::endl;
}

void CuboRubik::printFaceMap(char face) {
    const auto& faceCubes = faceMap[string(1, face)];
    std::cout << "Face " << face << " map:" << std::endl;
    for (int i = 0; i < 9; i++) {
        std::cout << faceCubes[i] << " ";
        if ((i + 1) % 3 == 0) std::cout << std::endl;
    }
}

void CuboRubik::printSliceMap(char slice) {
    const auto& sliceCubes = sliceMap[string(1, slice)];
    std::cout << "Slice " << slice << " map:" << std::endl;
    for (int i = 0; i < 8; i++) {
        std::cout << sliceCubes[i] << " ";
    }
    std::cout << std::endl;
}

// temp function to transform cube
void CuboRubik::applyTransform(matriz4x4 m) {
    for (const auto& cubePair : cubeMap) {
        cubePair.second->applyTransform(m);
    }
}

///// ROTACIONEESSS cristian
void CuboRubik::trasladarCuboGlobal(float tx, float ty, float tz) {
    acumTx += tx; acumTy += ty; acumTz += tz; // Solo acumulamos el valor
}

void CuboRubik::rotarCuboGlobalX(float angulo) {
    acumRotX += angulo; // Solo acumulamos el valor
}

void CuboRubik::rotarCuboGlobalY(float angulo) {
    acumRotY += angulo; // Solo acumulamos el valor
}

/*
std::vector<string> scrambleCube(int numMoves) {
    std::vector<std::string> sequenceString;
    std::vector<Move> scrambleSequence = generateScrambleSequence(numMoves);

    isExecutingSequence = true;
    emptyMoveQueue();
    
    std::cout << "Desordenando cubo..." << scrambleSequence.size() <<std::endl;
    for (const Move& move : scrambleSequence) {
        std::cout << move.face << "(" << move.angle << ") ";
    
        // if (move.isSlice) {
        //     rotateSlice(move.face, move.angle);
        // } else {
        //     rotateFace(move.face, move.angle);
        // }
        //currentAnimation.animationSpeed = 720.0f;
        if(move.angle < 0)
        {
            switch(move.face) {
                case 'U':
                    queueRotation('U', -90.0f);
                    sequenceString.push_back("U");
                    //rotateU();
                    //sequenceString.push_back("U");
                    break;
                case 'L':
                    queueRotation('L', -90.0f);
                    sequenceString.push_back("L");
                    //rotateL();
                    //sequenceString.push_back("L");
                    break;
                case 'F':
                    queueRotation('F', -90.0f);
                    sequenceString.push_back("F");
                    //rotateF();
                    //sequenceString.push_back("F");
                    break;
                case 'R':
                    queueRotation('R', -90.0f);
                    sequenceString.push_back("R");
                    //rotateR();
                    //sequenceString.push_back("R");
                    break;
                case 'B':
                    queueRotation('B', -90.0f);
                    sequenceString.push_back("B");
                    //rotateB();
                    //sequenceString.push_back("B");
                    break;
                case 'D':
                    queueRotation('D', -90.0f);
                    sequenceString.push_back("D");
                    //rotateD();
                    //sequenceString.push_back("D");
                    break;
                case 'V':
                    rotateSV();
                    break;
                case 'H':
                    rotateSH();
                    break;
                case 'S':
                    rotateSS();
                    break;
            }
        }
        else{
            switch(move.face) {
                case 'U':
                    queueRotation('U', 90.0f);
                    sequenceString.push_back("U'");	
                    //rotateUPrime();
                    //sequenceString.push_back("U'");
                    break;
                case 'L':
                    queueRotation('L', 90.0f);
                    sequenceString.push_back("L'");
                    //rotateLPrime();
                    //sequenceString.push_back("L'");
                    break;
                case 'F':
                    queueRotation('F', 90.0f);
                    sequenceString.push_back("F'");
                    //rotateFPrime();
                    //sequenceString.push_back("F'");
                    break;
                case 'R':
                    queueRotation('R', 90.0f);
                    sequenceString.push_back("R'");
                    //rotateRPrime();
                    //sequenceString.push_back("R'");
                    break;
                case 'B':
                    queueRotation('B', 90.0f);
                    sequenceString.push_back("B'");
                    //rotateBPrime();
                    //sequenceString.push_back("B'");
                    break;
                case 'D':
                    queueRotation('D', 90.0f);
                    sequenceString.push_back("D'");
                    //rotateDPrime();
                    //sequenceString.push_back("D'");
                    break;
                case 'V':
                    rotateSV();
                    break;
                case 'H':
                    rotateSH();
                    break;
                case 'S':
                    rotateSS();
                    break;
            }
        }
        
    }
    //std::cout << "queue size: "<< moveQueue.size() << std::endl;
    std::cout << std::endl;
    return sequenceString;
}
*/

CuboRubik::~CuboRubik() {
    /*
        // Cleanup buffers
        for (const auto& cube : cubeBuffers) {
            glDeleteVertexArrays(1, &cube.second.VAO);
            glDeleteBuffers(1, &cube.second.VBO);
        }
            */
    }
		

#endif // RUBIK_H_
