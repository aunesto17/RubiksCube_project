#ifndef THRUSTER_H_
#define THRUSTER_H_

#include <vector>
#include <cmath>
#include <cstdlib>
#include "helper.h"

class ThrusterEffect {
public:
    struct ThrusterParticle {
        vec3 position;
        vec3 velocity;
        float lifetime;
        float maxLifetime;
        float size;
        vec3 startColor;
        vec3 endColor;
    };

    std::vector<ThrusterParticle> particles;
    bool isThrusting = false;

    float exhaustBackOffset = 0.5f;
    float exhaustLateralOffset = 0.1f;
    float particleSpeed = 4.0f;
    float coneAngleDeg = 20.0f;
    int particlesPerExhaustPerFrame = 2;
    float sizeMultiplier = 0.8f;

    static unsigned int sharedVAO, sharedVBO, sharedEBO;
    static int sharedIndexCount;
    static bool isMeshLoaded;

    void init() {
        if (isMeshLoaded) return;

        float cubeVerts[] = {
            // Top face (+Y)  - normal(0,1,0)
            -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            // Bottom face (-Y)  - normal(0,-1,0)
            -0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            // Front face (+Z)  - normal(0,0,1)
            -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            // Back face (-Z)  - normal(0,0,-1)
             0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            // Right face (+X)  - normal(1,0,0)
             0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            // Left face (-X)  - normal(-1,0,0)
            -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,
        };

        unsigned short cubeIndices[] = {
             0,  1,  2,   2,  3,  0,
             4,  5,  6,   6,  7,  4,
             8,  9, 10,  10, 11,  8,
            12, 13, 14,  14, 15, 12,
            16, 17, 18,  18, 19, 16,
            20, 21, 22,  22, 23, 20
        };

        sharedIndexCount = 36;

        glGenVertexArrays(1, &sharedVAO);
        glGenBuffers(1, &sharedVBO);
        glGenBuffers(1, &sharedEBO);

        glBindVertexArray(sharedVAO);

        glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sharedEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

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

        std::cout << "[Thruster] Mesh initialized: 24 vertices, 12 triangles" << std::endl;
    }

    void emit(const vec3& shipPos, const vec3& forward, const vec3& right) {
        if (!isThrusting) return;

        vec3 backward = forward * -1.0f;

        float coneRad = helper::toRadians(coneAngleDeg);
        float maxSpread = sinf(coneRad);

        vec3 leftExhaust = shipPos + backward * exhaustBackOffset + right * exhaustLateralOffset;
        vec3 rightExhaust = shipPos + backward * exhaustBackOffset - right * exhaustLateralOffset;

        for (int exhaust = 0; exhaust < 2; exhaust++) {
            vec3 origin = (exhaust == 0) ? leftExhaust : rightExhaust;

            for (int i = 0; i < particlesPerExhaustPerFrame; i++) {
                ThrusterParticle p;
                p.position = origin;

                vec3 up = helper::normalize(helper::crossProduct(forward, right));
                float theta = ((float)(rand() % 1000) / 1000.0f) * 2.0f * (float)M_PI;
                float spread = ((float)(rand() % 1000) / 1000.0f) * maxSpread;
                float spreadR = spread * cosf(theta);
                float spreadU = spread * sinf(theta);

                p.velocity.x = backward.x + right.x * spreadR + up.x * spreadU;
                p.velocity.y = backward.y + right.y * spreadR + up.y * spreadU;
                p.velocity.z = backward.z + right.z * spreadR + up.z * spreadU;

                float len = sqrtf(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y + p.velocity.z * p.velocity.z);
                float speed = particleSpeed * (0.8f + ((float)(rand() % 100) / 100.0f) * 0.4f);
                if (len > 0.0f) {
                    p.velocity.x = (p.velocity.x / len) * speed;
                    p.velocity.y = (p.velocity.y / len) * speed;
                    p.velocity.z = (p.velocity.z / len) * speed;
                }

                p.maxLifetime = 0.1f + ((float)(rand() % 100) / 100.0f) * 0.1f;
                p.lifetime = p.maxLifetime;
                p.size = (0.04f + ((float)(rand() % 100) / 100.0f) * 0.04f) * sizeMultiplier;

                p.startColor = vec3(1.0f, 0.25f, 0.05f);
                p.endColor = vec3(1.0f, 0.85f, 0.1f);

                particles.push_back(p);
            }
        }
    }

    void update(float deltaTime) {
        for (size_t i = 0; i < particles.size(); ) {
            particles[i].lifetime -= deltaTime;
            if (particles[i].lifetime <= 0.0f) {
                particles.erase(particles.begin() + i);
                continue;
            }
            particles[i].position.x += particles[i].velocity.x * deltaTime;
            particles[i].position.y += particles[i].velocity.y * deltaTime;
            particles[i].position.z += particles[i].velocity.z * deltaTime;
            i++;
        }
    }

    void draw(unsigned int shaderProgram, GLint normalMatrixLoc, unsigned int transparentTexID) {
        if (particles.empty() || !isMeshLoaded) return;

        glUseProgram(shaderProgram);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, transparentTexID);
        glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

        glUniform1i(glGetUniformLocation(shaderProgram, "isEmissive"), GL_TRUE);

        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint emissiveColorLoc = glGetUniformLocation(shaderProgram, "emissiveColor");

        for (const auto& p : particles) {
            float t = p.lifetime / p.maxLifetime;

            vec3 color;
            color.x = (p.endColor.x + (p.startColor.x - p.endColor.x) * t) * t;
            color.y = (p.endColor.y + (p.startColor.y - p.endColor.y) * t) * t;
            color.z = (p.endColor.z + (p.startColor.z - p.endColor.z) * t) * t;

            float s = p.size * t;

            matriz4x4 model;
            model.mat = {
                s,    0.0f, 0.0f, p.position.x,
                0.0f, s,    0.0f, p.position.y,
                0.0f, 0.0f, s,    p.position.z,
                0.0f, 0.0f, 0.0f, 1.0f
            };

            std::array<float, 9> normalMat = helper::extract_normal_matrix(model);
            glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data());
            glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMat.data());
            glUniform3f(emissiveColorLoc, color.x, color.y, color.z);

            glBindVertexArray(sharedVAO);
            glDrawElements(GL_TRIANGLES, sharedIndexCount, GL_UNSIGNED_SHORT, 0);
        }

        glUniform1i(glGetUniformLocation(shaderProgram, "isEmissive"), GL_FALSE);
        glUniform3f(emissiveColorLoc, 0.0f, 0.0f, 0.0f);
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(0);
    }
};

unsigned int ThrusterEffect::sharedVAO = 0;
unsigned int ThrusterEffect::sharedVBO = 0;
unsigned int ThrusterEffect::sharedEBO = 0;
int ThrusterEffect::sharedIndexCount = 0;
bool ThrusterEffect::isMeshLoaded = false;

#endif // THRUSTER_H_
