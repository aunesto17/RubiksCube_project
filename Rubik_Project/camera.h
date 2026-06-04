#ifndef CAMERA_H_
#define CAMERA_H_

#include "matriz.h"
#include "vertex.h"
#include <cmath>
#include "helper.h"

class Camera {
private:
    enum class CameraAnimState {
        NONE,
        ZOOM_OUT,
        ROTATING,
        ZOOM_IN
    };

    class CameraAnimation {
        public:
        bool isAnimating = false;
        CameraAnimState state = CameraAnimState::NONE;
        float startYaw = 45.0f;
        float targetYaw = 45.0f;
        float startDistance = 10.0f;
        float targetDistance = 10.0f;
        float animationDuration = 1.0f;
        float elapsedTime = 0.0f;
        float rotationSpeed = 0.0f; // degrees per second
    };
    /*
    // Fixed isometric view parameters
    float distance = 15.0f;  // Fixed distance
    float pitch = 35.264f;   // True isometric pitch
    float yaw = 45.0f;       // True isometric yaw
    float fov = 45.0f;       // Standard field of view
    */
    // Fixed isometric view parameters
    float distance;
    float pitch;
    float yaw; 
    float fov; 

    vec3 positionOffset;

    // Internal Math helpers for vec3 (LookAt & Perspective require these)
    vec3 crossProduct(const vec3& a, const vec3& b) {
        return vec3(
            a.getY() * b.getZ() - a.getZ() * b.getY(),
            a.getZ() * b.getX() - a.getX() * b.getZ(),
            a.getX() * b.getY() - a.getY() * b.getX()
        );
    }

    // --- Movement speeds ---
    float orbitSpeed;       
    float zoomSpeed;        
    float translationSpeed;

    // --- Hard limits ---
    static constexpr float MIN_DISTANCE = 3.0f;
    static constexpr float MAX_DISTANCE = 40.0f;
    static constexpr float MIN_FOV = 1.0f;
    static constexpr float MAX_FOV = 90.0f;
    static constexpr float MAX_PITCH = 89.9f;
    static constexpr float MIN_PITCH = -89.9f;

    CameraAnimation cameraAnim;

    // --- Validation Helpers ---
    void clampPitchWithWarning(float& pitchValue) const {
        if (pitchValue > MAX_PITCH) {
            std::cout << "[Camera][WARNING] Pitch clamped to max." << std::endl;
            pitchValue = MAX_PITCH;
        } else if (pitchValue < MIN_PITCH) {
            std::cout << "[Camera][WARNING] Pitch clamped to min." << std::endl;
            pitchValue = MIN_PITCH;
        }
    }

    void clampDistanceWithWarning(float& distValue) const {
        if (distValue < MIN_DISTANCE) {
            distValue = MIN_DISTANCE;
        } else if (distValue > MAX_DISTANCE) {
            distValue = MAX_DISTANCE;
        }
    }

    void normalizeYaw(float& yawValue) const {
        while (yawValue < 0.0f) yawValue += 360.0f;
        while (yawValue >= 360.0f) yawValue -= 360.0f;
    }

public: 
    // isometric as default
    Camera(float initDist = 15.0f, float initPitch = 35.264f, float initYaw = 45.0f, float initFov = 45.0f)
        : distance(initDist), 
          pitch(initPitch), 
          yaw(initYaw), 
          fov(initFov),
          positionOffset(0.0f, 0.0f, 0.0f),
          orbitSpeed(90.0f),
          zoomSpeed(10.0f),
          translationSpeed(10.0f) 
    {
        clampPitchWithWarning(pitch);
        clampDistanceWithWarning(distance);
    }
    
    // Getters
    float getDistance() const { return distance; }
    float getPitch() const { return pitch; }
    float getYaw() const { return yaw; }
    float getFov() const { return fov; }
    float getMaxDistance() const { return MAX_DISTANCE; }

    // --- Setters ---
    void setDistance(float d) { distance = d; clampDistanceWithWarning(distance); }
    void setPitch(float p) { pitch = p; clampPitchWithWarning(pitch); }
    void setYaw(float y) { yaw = y; normalizeYaw(yaw); }
    void setFov(float f) { fov = helper::clamp(f, MIN_FOV, MAX_FOV); }
    
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
        float tanHalfFov = std::tan(helper::toRadians(fov) / 2.0f);

        matriz4x4 projMat;
        projMat.mat = {
            1.0f / (aspectRatio * tanHalfFov), 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f / tanHalfFov, 0.0f, 0.0f,
            0.0f, 0.0f, -(zFar + zNear) / (zFar - zNear), -(2.0f * zFar * zNear) / (zFar - zNear),
            0.0f, 0.0f, -1.0f, 0.0f
        };

        return projMat;
    }

    // --- Orbital Movement ---
    void orbitUp(float deltaTime) { pitch += orbitSpeed * deltaTime; clampPitchWithWarning(pitch); }
    void orbitDown(float deltaTime) { pitch -= orbitSpeed * deltaTime; clampPitchWithWarning(pitch); }
    void orbitLeft(float deltaTime) { yaw -= orbitSpeed * deltaTime; normalizeYaw(yaw); }
    void orbitRight(float deltaTime) { yaw += orbitSpeed * deltaTime; normalizeYaw(yaw); }

    void zoomIn(float deltaTime) { distance -= zoomSpeed * deltaTime; clampDistanceWithWarning(distance); }
    void zoomOut(float deltaTime) { distance += zoomSpeed * deltaTime; clampDistanceWithWarning(distance); }

    // --- Translation (Panning) ---
    void translateLeft(float deltaTime) { positionOffset.x -= translationSpeed * deltaTime; }
    void translateRight(float deltaTime) { positionOffset.x += translationSpeed * deltaTime; }
    void translateUp(float deltaTime) { positionOffset.y += translationSpeed * deltaTime; }
    void translateDown(float deltaTime) { positionOffset.y -= translationSpeed * deltaTime; }

    // --- Original Dummy Method Mappings ---
    // These ensure main.cpp continues to compile perfectly, linking to the new dynamic movement.
    void moveForward(float deltaTime) { orbitUp(deltaTime); }
    void moveBackward(float deltaTime) { orbitDown(deltaTime); }
    void moveLeft(float deltaTime) { orbitLeft(deltaTime); }
    void moveRight(float deltaTime) { orbitRight(deltaTime); }
    
    void setMoveSpeed(float speed) { translationSpeed = speed; }
    void setRotateSpeed(float speed) { orbitSpeed = speed; }
    void setZoomSpeed(float speed) { zoomSpeed = speed; }
    
    void reset() {
        distance = 15.0f;
        pitch = 35.264f;
        yaw = 45.0f;
        fov = 45.0f;
        positionOffset = vec3(0.0f, 0.0f, 0.0f);
        cameraAnim = CameraAnimation();
    }
    
    // --- Animation System ---
    void startCameraAnimation(float cubeDuration) {
        if (cubeDuration <= 0.0f) return;

        cameraAnim.isAnimating = true;
        cameraAnim.state = CameraAnimState::ZOOM_OUT;

        // Reset to known good starting positions
        pitch = 25.0f;
        yaw = 45.0f;
        distance = 10.0f;

        cameraAnim.startYaw = yaw;
        cameraAnim.startDistance = distance;
        cameraAnim.targetDistance = 25.0f;
        cameraAnim.targetYaw = yaw + 360.0f;
        cameraAnim.animationDuration = 1.0f;
        cameraAnim.elapsedTime = 0.0f;
        cameraAnim.rotationSpeed = 360.0f / cubeDuration;
    }

    void updateCameraAnimation(float deltaTime) {
        if (!cameraAnim.isAnimating) return;

        cameraAnim.elapsedTime += deltaTime;

        switch (cameraAnim.state) {
            case CameraAnimState::ZOOM_OUT: {
                float t = cameraAnim.elapsedTime / cameraAnim.animationDuration;
                if (t >= 1.0f) {
                    distance = cameraAnim.targetDistance;
                    cameraAnim.state = CameraAnimState::ROTATING;
                    cameraAnim.elapsedTime = 0.0f;
                } else {
                    // Ease-out
                    float ease_t = 1.0f - (1.0f - t) * (1.0f - t);
                    distance = helper::mix(cameraAnim.startDistance, cameraAnim.targetDistance, ease_t);
                }
                break;
            }
            case CameraAnimState::ROTATING: {
                yaw += cameraAnim.rotationSpeed * deltaTime;
                if (yaw >= cameraAnim.targetYaw) {
                    yaw = cameraAnim.startYaw;
                    cameraAnim.isAnimating = false;
                }
                break;
            }
            default:
                break;
        }
    }
    
    bool isAnimating() const { return cameraAnim.isAnimating; }
};

#endif // CAMERA_H_
