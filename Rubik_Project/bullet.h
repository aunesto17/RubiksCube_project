#ifndef BULLET_H_
#define BULLET_H_

#include <iostream>
#include <cmath>
#include <vector>
#include "helper.h"

class Bullet {
public:
    vec3 position{0.0f, 0.0f, 0.0f};
    vec3 direction{0.0f, 0.0f, -1.0f};
    float speed = 30.0f;
    float lifetime = 0.0f;
    float collisionRadius = 0.15f;

    static unsigned int sharedVAO, sharedVBO, sharedEBO;
    static int sharedIndexCount;
    static bool isMeshLoaded;
    static float bulletVisualRadius;
    static float maxLifetime;

    static float colorR;
    static float colorG;
    static float colorB;

    // Genera una esfera UV compartida para todas las balas
    static void initMesh(float radius = 1.0f, int stacks = 12, int sectors = 20) {
        std::vector<float> vertexData;
        std::vector<unsigned short> indices;

        for (int i = 0; i <= stacks; i++) {
            float phi = (float)M_PI * i / stacks;
            for (int j = 0; j <= sectors; j++) {
                float theta = 2.0f * (float)M_PI * j / sectors;

                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);

                // Position (location 0)
                vertexData.push_back(x);
                vertexData.push_back(y);
                vertexData.push_back(z);

                // Normal (location 1) - for a sphere, normal = normalized position
                float len = sqrtf(x*x + y*y + z*z);
                if (len > 0.0f) {
                    vertexData.push_back(x / len);
                    vertexData.push_back(y / len);
                    vertexData.push_back(z / len);
                } else {
                    vertexData.push_back(0.0f);
                    vertexData.push_back(1.0f);
                    vertexData.push_back(0.0f);
                }

                // Color #c2dde4 (location 2)
                vertexData.push_back(colorR);
                vertexData.push_back(colorG);
                vertexData.push_back(colorB);

                // Texture coordinates (location 3)
                float u = (float)j / sectors;
                float v = (float)i / stacks;
                vertexData.push_back(u);
                vertexData.push_back(v);
            }
        }

        for (int i = 0; i < stacks; i++) {
            for (int j = 0; j < sectors; j++) {
                int cur  = i * (sectors + 1) + j;
                int next = cur + sectors + 1;

                indices.push_back((unsigned short)cur);
                indices.push_back((unsigned short)next);
                indices.push_back((unsigned short)(next + 1));

                indices.push_back((unsigned short)cur);
                indices.push_back((unsigned short)(next + 1));
                indices.push_back((unsigned short)(cur + 1));
            }
        }

        sharedIndexCount = (int)indices.size();

        glGenVertexArrays(1, &sharedVAO);
        glGenBuffers(1, &sharedVBO);
        glGenBuffers(1, &sharedEBO);

        glBindVertexArray(sharedVAO);

        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

        // New layout: position(3) + normal(3) + color(3) + texCoord(2) = 11 floats, stride = 44 bytes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
        isMeshLoaded = true;

        std::cout << "[Bullet] Mesh initialized: " << vertexData.size() / 11 << " vertices, "
                  << sharedIndexCount / 3 << " triangles" << std::endl;
    }

    Bullet(vec3 startPos, vec3 dir) {
        position = startPos;
        direction = dir;
    }

    void update(float deltaTime) {
        position.x += direction.x * speed * deltaTime;
        position.y += direction.y * speed * deltaTime;
        position.z += direction.z * speed * deltaTime;
        lifetime += deltaTime;
    }

    bool isExpired() const {
        return lifetime > maxLifetime;
    }

    float getCollisionRadius() const { return collisionRadius; }

    void draw(unsigned int shaderProgram) {
        if (!isMeshLoaded) return;

        matriz4x4 model = getModelMatrix();
        std::array<float, 9> normalMat = helper::extract_normal_matrix(model);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMat.data());

        glBindVertexArray(sharedVAO);
        glDrawElements(GL_TRIANGLES, sharedIndexCount, GL_UNSIGNED_SHORT, 0);
    }

public:
    matriz4x4 getModelMatrix() {
        matriz4x4 m;
        float s = bulletVisualRadius;
        m.mat = {
            s,    0.0f, 0.0f, position.x,
            0.0f, s,    0.0f, position.y,
            0.0f, 0.0f, s,    position.z,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        return m;
    }
};

// Definicion de las variables estaticas compartidas
unsigned int Bullet::sharedVAO = 0;
unsigned int Bullet::sharedVBO = 0;
unsigned int Bullet::sharedEBO = 0;
int Bullet::sharedIndexCount = 0;
bool Bullet::isMeshLoaded = false;
float Bullet::bulletVisualRadius = 0.1f;
float Bullet::maxLifetime = 3.0f;
float Bullet::colorR = 0.761f;
float Bullet::colorG = 0.867f;
float Bullet::colorB = 0.894f;

#endif // BULLET_H_