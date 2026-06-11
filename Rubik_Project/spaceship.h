#ifndef SPACESHIP_H_
#define SPACESHIP_H_

#include <iostream>
#include "helper.h"
#include "3Dsloader.h"

class Spaceship {
public:
    vec3 position{0.0f, 0.0f, 0.0f};
    float scale = 1.0f;

    bool load(const char* filepath) {
        Mesh3DS mesh;
        if (!Load3DS(mesh, filepath)) return false;

        float size = computeSize(mesh);
        scale = 2.0f / size;
        std::cout << "[Spaceship] Auto-scale: " << scale << " (model size: " << size << ")" << std::endl;

        std::vector<float> vertexData;
        vertexData.reserve(mesh.vertices.size() * 8);
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const vec3& v = mesh.vertices[i];
            vertexData.push_back(v.getX());
            vertexData.push_back(v.getY());
            vertexData.push_back(v.getZ());
            vertexData.push_back(0.6f); vertexData.push_back(0.6f); vertexData.push_back(0.7f);
            if (i < mesh.texCoords.size()) {
                vertexData.push_back(mesh.texCoords[i].getX());
                vertexData.push_back(mesh.texCoords[i].getY());
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }

        indexCount = (int)mesh.indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned short), mesh.indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        m_loaded = true;
        std::cout << "[Spaceship] Ready: " << mesh.vertices.size() << " vertices, " << indexCount / 3 << " triangles" << std::endl;
        return true;
    }

    void draw(unsigned int shaderProgram, const matriz4x4& view, const matriz4x4& proj) {
        if (!m_loaded) return;

        glUseProgram(shaderProgram);

        matriz4x4 model = getModelMatrix();
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data());
        glUniformMatrix4fv(viewLoc, 1, GL_TRUE, view.mat.data());
        glUniformMatrix4fv(projLoc, 1, GL_TRUE, proj.mat.data());

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

    void moveX(float delta) { position.x += delta; }
    void moveY(float delta) { position.y += delta; }
    void moveZ(float delta) { position.z += delta; }
    void setPosition(const vec3& pos) { position = pos; }
    vec3 getPosition() const { return position; }

    ~Spaceship() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;
    bool m_loaded = false;

    float computeSize(const Mesh3DS& mesh) {
        if (mesh.vertices.empty()) return 1.0f;
        float minX = mesh.vertices[0].getX(), maxX = minX;
        float minY = mesh.vertices[0].getY(), maxY = minY;
        float minZ = mesh.vertices[0].getZ(), maxZ = minZ;
        for (const auto& v : mesh.vertices) {
            if (v.getX() < minX) minX = v.getX();
            if (v.getX() > maxX) maxX = v.getX();
            if (v.getY() < minY) minY = v.getY();
            if (v.getY() > maxY) maxY = v.getY();
            if (v.getZ() < minZ) minZ = v.getZ();
            if (v.getZ() > maxZ) maxZ = v.getZ();
        }
        float dx = maxX - minX;
        float dy = maxY - minY;
        float dz = maxZ - minZ;
        float maxDim = dx > dy ? dx : dy;
        maxDim = maxDim > dz ? maxDim : dz;
        return maxDim > 0.0f ? maxDim : 1.0f;
    }

    matriz4x4 getModelMatrix() {
        matriz4x4 m;
        m.mat = {
            scale, 0.0f,  0.0f,  position.x,
            0.0f,  scale, 0.0f,  position.y,
            0.0f,  0.0f,  scale, position.z,
            0.0f,  0.0f,  0.0f,  1.0f
        };
        return m;
    }
};

#endif // SPACESHIP_H_
