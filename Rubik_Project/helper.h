#ifndef HELPER_H_
#define HELPER_H_

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"	// libreria para cargar imagenes
#include "solver/solve.h"
#include "solver/random.h"
#include "vertex.h"
#include "matriz.h"
#include "transform.h"

#include <filesystem>
#include <fstream>
#include <queue>
#include <deque>
#include <cmath>   // For sin, cos, tan, sqrt
#include <cstring> // For memcpy
#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Matrix locations
GLint viewLoc;
GLint projLoc;
GLint modelLoc;
GLint viewLocSB;
GLint projLocSB;

// --- Lighting uniform locations (Phase 1+2) ---
GLint lightDirLoc;
GLint lightColorLoc;
GLint lightIntensityLoc;
GLint ambientStrengthLoc;
GLint ambientColorLoc;
GLint viewPosLoc;
GLint normalMatrixLoc;

// Point light uniform locations
GLint pointLightPosLoc[2];
GLint pointLightColorLoc[2];
GLint pointLightIntensityLoc[2];
GLint pointLightConstantLoc[2];
GLint pointLightLinearLoc[2];
GLint pointLightQuadraticLoc[2];
GLint numPointLightsLoc;

// helper functions and classes

namespace helper {

//static inline Mat4 mul(const Mat4& A, const Mat4& B);

inline float radians(float degrees) {
    return degrees * (float)M_PI / 180.0f;
}

inline float toRadians(float degrees) {
    return degrees * (M_PI / 180.0f);
}

inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline float mix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}


static inline float dotProduct(const vec3& a, const vec3& b) {
    return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}

static inline vec3 crossProduct(const vec3& a, const vec3& b) {
    return {((a.y * b.z) - (a.z * b.y)), ((a.z * b.x) - (a.x * b.z)), ((a.x * b.y - a.y * b.x))};
}


static inline float length(const vec3& v) {
    return (std::sqrt(dotProduct(v, v)));
}

static inline vec3 normalize(const vec3& v) {
    float L = length(v);
    return ((L > 0) ? (v * (1.0f / L)) : v);
}

static inline matriz4x4 rotAxisAngle(const vec3& axis, float radians)
{
    vec3 a = normalize(axis);
    float c = std::cos(radians), s = std::sin(radians), t = 1.0f - c;
    float x=a.x, y=a.y, z=a.z;
    matriz4x4 R;
    R.mat[0]  = t*x*x + c;     R.mat[4]  = t*x*y - s*z; R.mat[8]  = t*x*z + s*y;
    R.mat[1]  = t*x*y + s*z;   R.mat[5]  = t*y*y + c;   R.mat[9]  = t*y*z - s*x;
    R.mat[2]  = t*x*z - s*y;   R.mat[6]  = t*y*z + s*x; R.mat[10] = t*z*z + c;
    return R;
}


static inline matriz4x4 getRotationMatrix(char face, float angle) {
    vec3 axis;
    switch(face) {
        // faces
        case 'U': case 'D': // Y-axis rotation
            axis = vec3(0.0f, 1.0f, 0.0f);
            if (face == 'D') angle = -angle;
            break;
        case 'L': case 'R': // X-axis rotation
            axis = vec3(1.0f, 0.0f, 0.0f);
            if (face == 'L') angle = -angle;
            break;
        case 'F': case 'B': // Z-axis rotation
            axis = vec3(0.0f, 0.0f, 1.0f);
            if (face == 'B') angle = -angle;
            break;
        case 'V': // Middle vertical slice (like L)
            axis = vec3(1.0f, 0.0f, 0.0f);
            //angle = -angle;
            break;
        case 'H': // Middle horizontal slice (like U)
            axis = vec3(0.0f, 1.0f, 0.0f);
            //angle = -angle;
            break;
            // slices   
        case 'S': // Middle slice (like F)
            axis = vec3(0.0f, 0.0f, 1.0f);
            break;
    }
    return rotAxisAngle(axis, radians(angle) );
}

// --- NORMAL COMPUTATION ---

// Compute per-vertex normals from indexed triangle data.
// Uses face-normal averaging: each vertex normal = average of adjacent face normals.
static inline std::vector<vec3> computeNormals(
    const std::vector<vec3>& vertices,
    const std::vector<unsigned short>& indices)
{
    std::vector<vec3> normals(vertices.size(), vec3(0.0f, 0.0f, 0.0f));
    std::vector<int> counts(vertices.size(), 0);

    // Iterate over triangles (3 indices per triangle)
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        unsigned short i0 = indices[i];
        unsigned short i1 = indices[i + 1];
        unsigned short i2 = indices[i + 2];

        vec3 v0 = vertices[i0];
        vec3 v1 = vertices[i1];
        vec3 v2 = vertices[i2];

        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;
        vec3 faceNormal = normalize(crossProduct(e1, e2));

        normals[i0] = normals[i0] + faceNormal;  counts[i0]++;
        normals[i1] = normals[i1] + faceNormal;  counts[i1]++;
        normals[i2] = normals[i2] + faceNormal;  counts[i2]++;
    }

    // Average and normalize
    for (size_t i = 0; i < normals.size(); i++) {
        if (counts[i] > 0) {
            normals[i] = normals[i] * (1.0f / static_cast<float>(counts[i]));
            normals[i] = normalize(normals[i]);
        }
    }

    return normals;
}

// --- NORMAL MATRIX EXTRACTION ---

// Extract the normal matrix from a matriz4x4.
// The normal matrix is the inverse-transpose of the upper 3x3.
// For rotation matrices (orthonormal), this is just the rotation basis.
// We orthonormalize the basis to handle any scale/shear.
// Returns a row-major 3x3 matrix suitable for glUniformMatrix3fv with GL_TRUE.
static inline std::array<float, 9> extractNormalMatrix(const matriz4x4& m) {
    std::array<float, 9> n;

    // Extract basis vectors from row-major storage
    float x_x = m.mat[0],  x_y = m.mat[1],  x_z = m.mat[2];
    float y_x = m.mat[4],  y_y = m.mat[5],  y_z = m.mat[6];
    float z_x = m.mat[8],  z_y = m.mat[9],  z_z = m.mat[10];

    // Normalize X axis
    float lenX = std::sqrt(x_x*x_x + x_y*x_y + x_z*x_z);
    if (lenX > 0.0001f) { x_x /= lenX; x_y /= lenX; x_z /= lenX; }

    // Make Y perpendicular to X
    float dotYX = y_x*x_x + y_y*x_y + y_z*x_z;
    y_x -= dotYX * x_x;  y_y -= dotYX * x_y;  y_z -= dotYX * x_z;
    float lenY = std::sqrt(y_x*y_x + y_y*y_y + y_z*y_z);
    if (lenY > 0.0001f) { y_x /= lenY; y_y /= lenY; y_z /= lenY; }

    // Make Z perpendicular to X and Y
    float dotZX = z_x*x_x + z_y*x_y + z_z*x_z;
    float dotZY = z_x*y_x + z_y*y_y + z_z*y_z;
    z_x -= dotZX*x_x + dotZY*y_x;
    z_y -= dotZX*x_y + dotZY*y_y;
    z_z -= dotZX*x_z + dotZY*y_z;
    float lenZ = std::sqrt(z_x*z_x + z_y*z_y + z_z*z_z);
    if (lenZ > 0.0001f) { z_x /= lenZ; z_y /= lenZ; z_z /= lenZ; }

    // Store orthonormalized basis as normal matrix
    // For orthonormal R: (R^-1)^T = R, so normal matrix = R
    n[0] = x_x;  n[1] = x_y;  n[2] = x_z;
    n[3] = y_x;  n[4] = y_y;  n[5] = y_z;
    n[6] = z_x;  n[7] = z_y;  n[8] = z_z;

    return n;
}



} // namespace helper

unsigned int loadTexture(const char* path) {
    // Print current working directory
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    std::cout << "Attempting to load texture: " << path << std::endl;

    // Check if file exists
    std::ifstream f(path);
    if (!f.good()) {
        std::cout << "Error: File does not exist!" << std::endl;
        return 0;
    }

    // Get file size
    f.seekg(0, std::ios::end);
    size_t fileSize = f.tellg();
    f.close();
    std::cout << "File size: " << fileSize << " bytes" << std::endl;

    // Flip textures if needed
    stbi_set_flip_vertically_on_load(true);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Load image data
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    
    if (data) {
        std::cout << "Image loaded successfully:" << std::endl;
        std::cout << "Width: " << width << std::endl;
        std::cout << "Height: " << height << std::endl;
        std::cout << "Channels: " << nrChannels << std::endl;
        
        // Print first few pixels of the image
            // std::cout << "First 16 bytes of image data:" << std::endl;
            // for(int i = 0; i < 16 && i < width * height * nrChannels; i++) {
            //     std::cout << (int)data[i] << " ";
            //     if((i + 1) % 4 == 0) std::cout << std::endl;
            // }

        GLenum internalFormat;
        GLenum dataFormat;
        if (nrChannels == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }
        else if (nrChannels == 3) {
            internalFormat = GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrChannels == 4) {
            internalFormat = GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        
        // Check for OpenGL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << "OpenGL error after texture creation: " << err << std::endl;
        }

        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        std::cout << "STB Error: " << stbi_failure_reason() << std::endl;
    }

    stbi_image_free(data);
    return texture;
}


// Also check if the files exist before trying to load them
void checkTextureFiles() {
    const char* textureFiles[] = {
        "letter-u.png",
        "letter-c.png",
        "letter-s.png",
        "letter-p.png",
        "ucsp-logo.png"
    };

    for (const char* file : textureFiles) {
        std::ifstream f(file);
        if (!f.good()) {
            std::cout << "Texture file not found: " << file << std::endl;
        } else {
            std::cout << "Found texture file: " << file << std::endl;
        }
    }
}

void snapToGrid(vec3& vertex, float gridSize = 1.02f) {
    // Round to nearest grid position
    vertex.x = round(vertex.x / gridSize) * gridSize;
    vertex.y = round(vertex.y / gridSize) * gridSize;
    vertex.z = round(vertex.z / gridSize) * gridSize;
}

// Add this helper function
float normalizeAngle(float angle) {
    // Normalize angle to -180 to 180 range
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

vec3 getCubeCenter(const std::vector<vec3>& vertices) {
    vec3 sum(0.0f, 0.0f, 0.0f);
    for (const auto& v : vertices) {
        sum = sum + vec3(v.getX(), v.getY(), v.getZ());
    }
    return sum / static_cast<float>(vertices.size());
}


#endif // HELPER_H_