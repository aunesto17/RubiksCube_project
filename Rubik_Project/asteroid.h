#ifndef ASTEROID_H_
#define ASTEROID_H_

#include <iostream>
#include <cmath>
#include <vector>
#include "helper.h"
#include "3dsloader.h"

class Asteroid {
public:
    vec3 position{0.0f, 0.0f, 0.0f};
    vec3 direction{0.0f, 0.0f, 0.0f};
    float speed = 4.5f;
    float scale = 1.0f;
    
    // Variables de rotación interna para que el asteroide gire sobre sí mismo
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
    float rotSpeedX = 0.0f, rotSpeedY = 0.0f, rotSpeedZ = 0.0f;

    // Variables estáticas para que TODOS los asteroides compartan la misma malla en memoria de GPU
    // ¡Esto optimiza el rendimiento drásticamente!
    static unsigned int sharedVAO, sharedVBO, sharedEBO;
    static int sharedIndexCount;
    static bool isMeshLoaded;

    // Inicializador de la malla base (se llama UNA SÓLA VEZ en el main)
    static bool loadMesh(const char* filepath) {
        Mesh3DS mesh; // Usando tu estructura de mallas actual
        if (!Load3DS(mesh, filepath)) return false; // Reemplazar por tu cargador de .obj si difiere

        // Calcular tamaño base
        float minX = mesh.vertices[0].getX(), maxX = minX;
        float minY = mesh.vertices[0].getY(), maxY = minY;
        float minZ = mesh.vertices[0].getZ(), maxZ = minZ;
        for (const auto& v : mesh.vertices) {
            if (v.getX() < minX) minX = v.getX(); if (v.getX() > maxX) maxX = v.getX();
            if (v.getY() < minY) minY = v.getY(); if (v.getY() > maxY) maxY = v.getY();
            if (v.getZ() < minZ) minZ = v.getZ(); if (v.getZ() > maxZ) maxZ = v.getZ();
        }
        float maxDim = (maxX - minX) > (maxY - minY) ? (maxX - minX) : (maxY - minY);
        maxDim = maxDim > (maxZ - minZ) ? maxDim : (maxZ - minZ);
        
        float baseScale = 1.5f / maxDim;

        // Compute per-vertex normals from face normals
        std::vector<vec3> normals = helper::computeNormals(mesh.vertices, mesh.indices);

        std::vector<float> vertexData;
        vertexData.reserve(mesh.vertices.size() * 11);
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const vec3& v = mesh.vertices[i];
            const vec3& n = normals[i];

            // 1. Position (location 0)
            vertexData.push_back(v.getX());
            vertexData.push_back(v.getY());
            vertexData.push_back(v.getZ());

            // 2. Normal (location 1)
            vertexData.push_back(n.getX());
            vertexData.push_back(n.getY());
            vertexData.push_back(n.getZ());

            // 3. Color (location 2)
            vertexData.push_back(0.6f);
            vertexData.push_back(0.6f);
            vertexData.push_back(0.7f);

            // 4. Texture coordinates (location 3)
            if (i < mesh.texCoords.size() && (mesh.texCoords[i].getX() != 0.0f || mesh.texCoords[i].getY() != 0.0f)) {
                vertexData.push_back(mesh.texCoords[i].getX());
                vertexData.push_back(mesh.texCoords[i].getY());
            } else {
                // Spherical UV fallback
                float length = sqrtf(v.getX()*v.getX() + v.getY()*v.getY() + v.getZ()*v.getZ());
                float u = 0.5f;
                float v_tex = 0.5f;
                if (length > 0.0f) {
                    float nx = v.getX() / length;
                    float ny = v.getY() / length;
                    float nz = v.getZ() / length;
                    u = 0.5f + (atan2f(nz, nx) / (2.0f * 3.141592f));
                    v_tex = 0.5f + (asinf(ny) / 3.141592f);
                }
                vertexData.push_back(u);
                vertexData.push_back(v_tex);
            }
        }

        sharedIndexCount = (int)mesh.indices.size();

        glGenVertexArrays(1, &sharedVAO);
        glGenBuffers(1, &sharedVBO);
        glGenBuffers(1, &sharedEBO);

        glBindVertexArray(sharedVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned short), mesh.indices.data(), GL_STATIC_DRAW);

        // Layout: 3 pos + 3 normal + 3 color + 2 texCoord = 11 floats (44 bytes)
        GLsizei stride = 11 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        isMeshLoaded = true;
		
        return true;
    }

    // Constructor para un asteroide individual con aleatoriedad
    Asteroid(vec3 startPos, vec3 targetPos, float sizeFactor) {
        position = startPos;
        scale = sizeFactor;

        // Calcular vector dirección normalizado: (Destino - Origen)
        vec3 dirRaw = vec3(targetPos.x - startPos.x, targetPos.y - startPos.y, targetPos.z - startPos.z);
        float length = sqrtf(dirRaw.x * dirRaw.x + dirRaw.y * dirRaw.y + dirRaw.z * dirRaw.z);
        if (length > 0.0f) {
            direction = vec3(dirRaw.x / length, dirRaw.y / length, dirRaw.z / length);
        }

        // Velocidades estocásticas de rotación (para que giren en el espacio de forma natural)
        rotSpeedX = ((float)(rand() % 100) / 100.0f) * 2.0f;
        rotSpeedY = ((float)(rand() % 100) / 100.0f) * 2.0f;
        rotSpeedZ = ((float)(rand() % 100) / 100.0f) * 2.0f;
    }

    // Actualiza la posición y las rotaciones internas basándose en el DeltaTime
    void update(float deltaTime) {
        position.x += direction.x * speed * deltaTime;
        position.y += direction.y * speed * deltaTime;
        position.z += direction.z * speed * deltaTime;

        rotX += rotSpeedX * deltaTime;
        rotY += rotSpeedY * deltaTime;
        rotZ += rotSpeedZ * deltaTime;
    }

    matriz4x4 getModelMatrix() {
        matriz4x4 m;
        float cx = cosf(rotX), sx = sinf(rotX);
        float cy = cosf(rotY), sy = sinf(rotY);

        float escalaCorrectiva = 0.001f; // tamaño a escala del asteroide
        float escalaFinal = scale * escalaCorrectiva;

        m.mat = {
            escalaFinal * cy,  -escalaFinal * sy * sx,  escalaFinal * sy * cx,  position.x,
            0.0f,               escalaFinal * cx,       escalaFinal * sx,       position.y,
            -escalaFinal * sy, -escalaFinal * cy * sx,  escalaFinal * cy * cx,  position.z,
            0.0f,               0.0f,                    0.0f,                    1.0f
        };
        return m;
    }

    // Draw with model matrix pre-uploaded by caller (for lighting normal matrix)
    void drawRaw(unsigned int shaderProgram) {
        if (!isMeshLoaded) return;
        glBindVertexArray(sharedVAO);
        glDrawElements(GL_TRIANGLES, sharedIndexCount, GL_UNSIGNED_SHORT, 0);
    }

    // Legacy draw that uploads its own model matrix
    void draw(unsigned int shaderProgram) {
        if (!isMeshLoaded) return;
        matriz4x4 model = getModelMatrix();
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data());
        drawRaw(shaderProgram);
    }
};

// Definición de las variables estáticas compartidas
unsigned int Asteroid::sharedVAO = 0;
unsigned int Asteroid::sharedVBO = 0;
unsigned int Asteroid::sharedEBO = 0;
int Asteroid::sharedIndexCount = 0;
bool Asteroid::isMeshLoaded = false;

#endif // ASTEROID_H_