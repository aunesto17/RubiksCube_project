#ifndef SPACESHIP_H_
#define SPACESHIP_H_

#include <iostream>
#include "helper.h"
#include "3Dsloader.h"

class Spaceship {
public:
    vec3 position{0.0f, 0.0f, 0.0f};
    float scale = 0.25f;
    float yaw = 0.0f;   // rotacion horizontal (grados), positivo = girar a la izquierda
    float pitch = 0.0f; // rotacion vertical (grados), positivo = nariz hacia arriba

    bool load(const char* filepath) {
        Mesh3DS mesh;
        if (!Load3DS(mesh, filepath)) {
            std::cerr << "[Spaceship] ERROR: Failed to load mesh from " << filepath << std::endl;
            return false;
        }

        float size = computeSize(mesh);
        scale = 1.0f / size;
        std::cout << "[Spaceship] Auto-scale: " << scale << " (model size: " << size << ")" << std::endl;

        // Compute per-vertex normals from the mesh geometry
        std::vector<vec3> normals = helper::compute_normals(mesh.vertices, mesh.indices);
        if (normals.empty()) {
            std::cerr << "[Spaceship] WARNING: Computed normals are empty. Using fallback normals." << std::endl;
            normals.resize(mesh.vertices.size(), vec3(0.0f, 0.0f, 1.0f));
        }

        std::vector<float> vertexData;
        vertexData.reserve(mesh.vertices.size() * 11);
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const vec3& v = mesh.vertices[i];
            const vec3& n = normals[i];

            // Position (location 0)
            vertexData.push_back(v.getX());
            vertexData.push_back(v.getY());
            vertexData.push_back(v.getZ());

            // Normal (location 1)
            vertexData.push_back(n.getX());
            vertexData.push_back(n.getY());
            vertexData.push_back(n.getZ());

            // Color (location 2)
            vertexData.push_back(0.6f); vertexData.push_back(0.6f); vertexData.push_back(0.7f);

            // TexCoord (location 3)
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

        m_loaded = true;
        std::cout << "[Spaceship] Ready: " << mesh.vertices.size() << " vertices, " << indexCount / 3 << " triangles" << std::endl;
        return true;
    }

    void draw(unsigned int shaderProgram, const matriz4x4& view, const matriz4x4& proj) {
        if (!m_loaded) return;

        glUseProgram(shaderProgram);

        matriz4x4 model = getModelMatrix();
        std::array<float, 9> normalMat = helper::extract_normal_matrix(model);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLint normalMatrixLoc = glGetUniformLocation(shaderProgram, "normalMatrix");
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(modelLoc, 1, GL_TRUE, model.mat.data());
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, normalMat.data());
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
    float getCollisionRadius() const { return collisionRadius; }

    // Calcula la direccion hacia adelante en world usando yaw y pitch
    vec3 getForward() const {
        float yawRad = helper::toRadians(yaw);
        float pitchRad = helper::toRadians(pitch);
        return vec3(-std::sin(yawRad) * std::cos(pitchRad),
                     std::sin(pitchRad),
                    -std::cos(yawRad) * std::cos(pitchRad));
    }

    // Calcula la direccion derecha en el plano horizontal (sin pitch)
    vec3 getRight() const {
        float yawRad = helper::toRadians(yaw);
        return vec3(std::cos(yawRad), 0.0f, -std::sin(yawRad));
    }

    // Calcula la direccion izquierda en el plano horizontal (sin pitch)
    vec3 getLeft() const {
        vec3 r = getRight();
        return vec3(-r.x, -r.y, -r.z);
    }

    // Mueve la nave hacia adelante en la direccion que apunta la punta de la nave
    void moveForward(float step) {
        vec3 fwd = getForward();
        position.x += fwd.x * step;
        position.y += fwd.y * step;
        position.z += fwd.z * step;
    }

    // Mueve la nave hacia atras (direccion opuesta a la punta de la nave)
    void moveBackward(float step) {
        vec3 fwd = getForward();
        position.x -= fwd.x * step;
        position.y -= fwd.y * step;
        position.z -= fwd.z * step;
    }

    ~Spaceship() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;
    bool m_loaded = false;
    float collisionRadius = 0.5f;

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
        collisionRadius = maxDim > 0.0f ? (maxDim * 0.5f * (1.0f / maxDim)) : 0.5f;
        return maxDim > 0.0f ? maxDim : 1.0f;
    }

public:
    // Construye la matriz modelo: T(posicion) × Ry(yaw) × Rx(pitch) × CorreccionModelo
    // CorreccionModelo transforma -Y del modelo (cockpit) a -Z (adelante en OpenGL)
    // y +Z del modelo (arriba) a +Y (arriba en OpenGL)
    matriz4x4 getModelMatrix() {
        float yawRad = helper::toRadians(yaw);
        float pitchRad = helper::toRadians(pitch);
        float cy = std::cos(yawRad), sy = std::sin(yawRad);
        float cp = std::cos(pitchRad), sp = std::sin(pitchRad);

        matriz4x4 model;

        // Matriz combinada: Traslacion × RotacionY × Escala
        model.mat = {
            scale * cy,   0.0f,       scale * sy,   position.x,
            0.0f,         scale,      0.0f,         position.y,
           -scale * sy,   0.0f,       scale * cy,   position.z,
            0.0f,         0.0f,       0.0f,         1.0f
        };

        // Rotacion de pitch (nariz arriba/abajo) alrededor del eje X local
        matriz4x4 rxPitch;
        rxPitch.mat = {
            1.0f,  0.0f,   0.0f,  0.0f,
            0.0f,  cp,    -sp,    0.0f,
            0.0f,  sp,     cp,    0.0f,
            0.0f,  0.0f,   0.0f,  1.0f
        };

        model.multMat(rxPitch);

        // Correccion del modelo 3DS: Rz(180°) × Rx(90°)
        // Transforma: -Y (cockpit) → -Z (adelante), +Z (arriba) → +Y (arriba)
        matriz4x4 modelCorr;
        modelCorr.mat = {
           -1.0f,  0.0f,  0.0f,  0.0f,
            0.0f,  0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,  0.0f,
            0.0f,  0.0f,  0.0f,  1.0f
        };

        model.multMat(modelCorr);

        return model;
    }
};

#endif // SPACESHIP_H_