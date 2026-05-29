#ifndef RUBIK_H_
#define RUBIK_H_

// rubik.h - class to store and manipulate the Rubik's cube

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
    void updateAdjacentFaces(char face, bool clockwise);
public:
    CuboRubik(float lastFrameTime, Camera & cam);
    void init(){
        initializeCubes();
    }

    void resetRubik();

    void draw(unsigned int shaderProgram);

    void rotateU();
    void rotateD();
    void rotateL();
    void rotateR();
    void rotateF();
    void rotateB();

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
            // 1. Tomamos los vértices limpios del origen (copia maestra)
            std::vector<vec3> verticesLimpios = pair.second->getVerticesLocales(); 
            
            // 2. Aplicamos la transformación en un ORDEN ESTRICTO que jamás cambia
            verticesLimpios = mRotY.multFig(verticesLimpios);
            verticesLimpios = mRotX.multFig(verticesLimpios);
            verticesLimpios = mTras.multFig(verticesLimpios);
            
            // 3. Guardamos el resultado en los vértices de renderizado y subimos a la GPU
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
    //if (currentAnimation.isAnimating) {
    //    return; // Don't start new animation while one is in progress
    //}

    angle = normalizeAngle(angle);

    affectedCubes.clear();
    if (faceMap.find(std::string(1, face)) != faceMap.end()) {
        const auto& faceCubes = faceMap[std::string(1, face)];
        affectedCubes.insert(
            affectedCubes.end(),
            faceCubes.begin(),
            faceCubes.end()
        );
    }
    

    updateCubePositions(affectedCubes, face, angle);
    updateFaceMapAfterRotation(face, angle < 0);

}

void CuboRubik::updateCubePositions(const std::vector<std::string>& affectedCubes, char face, float angle) {
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
            rotation = t.rotacionX(physAngle); 
            break;
        case 'U': 
        case 'D': 
            rotation = t.rotacionY(physAngle); 
            break;
        case 'F': 
        case 'B': 
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

void CuboRubik::rotateU() {
    rotateFace('U', -90.0f);
}

void CuboRubik::rotateL() {
    rotateFace('L', -90.0f);
}


void CuboRubik::rotateF() {
    rotateFace('F', -90.0f);
}


void CuboRubik::rotateR() {
    rotateFace('R', -90.0f);
}

void CuboRubik::rotateB() {
    rotateFace('B', -90.0f);
}

void CuboRubik::rotateD() {
    rotateFace('D', -90.0f);
}


void CuboRubik::resetRubik() {
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