#ifndef CAMERA_H_
#define CAMERA_H_

#include "matriz.h"
#include "vertex.h"
#include <cmath>
#include "helper.h"

class Camera {
private:
    // Fixed isometric view parameters
    float distance = 15.0f;  // Fixed distance
    float pitch = 35.264f;   // True isometric pitch
    float yaw = 45.0f;       // True isometric yaw
    float fov = 45.0f;       // Standard field of view

    // Internal Math helpers for vec3 (LookAt & Perspective require these)
    vec3 crossProduct(const vec3& a, const vec3& b) {
        return vec3(
            a.getY() * b.getZ() - a.getZ() * b.getY(),
            a.getZ() * b.getX() - a.getX() * b.getZ(),
            a.getX() * b.getY() - a.getY() * b.getX()
        );
    }

    float dotProduct(const vec3& a, const vec3& b) {
        return a.getX() * b.getX() + a.getY() * b.getY() + a.getZ() * b.getZ();
    }

    vec3 normalize(vec3 v) {
        float length = std::sqrt(v.getX() * v.getX() + v.getY() * v.getY() + v.getZ() * v.getZ());
        if (length > 0.0f) {
            return vec3(v.getX() / length, v.getY() / length, v.getZ() / length);
        }
        return v;
    }

    float toRadians(float degrees) {
        return degrees * M_PI / 180.0f;
    }

public: 
    Camera() {}
    
    // Getters
    float getDistance() const { return distance; }
    float getPitch() const { return pitch; }
    float getYaw() const { return yaw; }
    float getFov() const { return fov; }
    
    // viewMatrix o LookAt
    matriz4x4 getViewMatrix() {
        // Calculate camera position based on fixed spherical coordinates
        float pitchRad = helper::toRadians(pitch);
        float yawRad = helper::toRadians(yaw);
        
        float x = distance * std::cos(pitchRad) * std::cos(yawRad);
        float y = distance * std::sin(pitchRad);
        float z = distance * std::cos(pitchRad) * std::sin(yawRad);

        // EYE
        vec3 eye(x, y, z);
        // C
        vec3 center(0.0f, 0.0f, 0.0f);
        // Up vector
        vec3 up(0.0f, 1.0f, 0.0f);

        // calculamos todo para LookAt
        vec3 w = helper::normalize(vec3(eye.getX() - center.getX(), 
                           eye.getY() - center.getY(), 
                           eye.getZ() - center.getZ()));
        vec3 u = helper::normalize(helper::crossProduct(up, w));
        vec3 v = helper::crossProduct(w, u);

        matriz4x4 viewMat;
        viewMat.mat = {
             u.getX(),  u.getY(),  u.getZ(), -helper::dotProduct(u, eye),
             v.getX(),  v.getY(),  v.getZ(), -helper::dotProduct(v, eye),
             w.getX(),  w.getY(),  w.getZ(), -helper::dotProduct(w, eye),
             0.0f,      0.0f,      0.0f,      1.0f
        };

        return viewMat;
    }

    // Get perspective projection matrix using matriz4x4
    matriz4x4 getPerspectiveMatrix(float aspectRatio) {
        float zNear = 0.1f;
        float zFar = 100.0f;
        float tanHalfFov = std::tan(toRadians(fov) / 2.0f);

        matriz4x4 projMat;
        projMat.mat = {
            1.0f / (aspectRatio * tanHalfFov), 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f / tanHalfFov, 0.0f, 0.0f,
            0.0f, 0.0f, -(zFar + zNear) / (zFar - zNear), -(2.0f * zFar * zNear) / (zFar - zNear),
            0.0f, 0.0f, -1.0f, 0.0f
        };

        return projMat;
    }

    // ---------------------------------------------------------
    // DUMMY METHODS (Ensures main.cpp continues to compile)
    // ---------------------------------------------------------
    void moveForward(float deltaTime) {}
    void moveBackward(float deltaTime) {}
    void moveLeft(float deltaTime) {}
    void moveRight(float deltaTime) {}
    void zoomIn(float deltaTime) {}
    void zoomOut(float deltaTime) {}
    
    void setMoveSpeed(float speed) {}
    void setRotateSpeed(float speed) {}
    void setZoomSpeed(float speed) {}
    
    void reset() {}
    
    void startCameraAnimation(float cubeDuration) {}
    void updateCameraAnimation(float deltaTime) {}
    bool isAnimating() const { return false; }
};

#endif // CAMERA_H_