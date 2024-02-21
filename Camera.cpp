#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        //TODO - Update the rest of camera parameters

    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {
        // Calculating the forward direction based on the camera's current orientation
        glm::vec3 forward = glm::normalize(cameraTarget - cameraPosition);
        // Calculating the right direction, perpendicular to both the forward direction and the camera's up direction
        glm::vec3 right = glm::normalize(glm::cross(forward, cameraUpDirection));

        // Update camera position and target based on the specified direction
        switch (direction) {
        case MOVE_LEFT:
            cameraPosition -= right * speed;
            cameraTarget -= right * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition += right * speed;
            cameraTarget += right * speed;
            break;
        case MOVE_BACKWARD:
            cameraPosition -= forward * speed;
            cameraTarget -= forward * speed;
            break;
        case MOVE_FORWARD:
            cameraPosition += forward * speed;
            cameraTarget += forward * speed;
            break;
        }
    }

    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        cameraTarget = cameraPosition + glm::normalize(direction);
    }

}