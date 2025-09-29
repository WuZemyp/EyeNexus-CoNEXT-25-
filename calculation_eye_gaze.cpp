#include "C:\Users\13513\ALVR_Private\ALVR\glm\glm\glm.hpp"
#include "C:\Users\13513\ALVR_Private\ALVR\glm\glm\gtc\quaternion.hpp"
#include "C:\Users\13513\ALVR_Private\ALVR\glm\glm\gtc\matrix_transform.hpp"
#include <cmath>
#include <math.h>
#include <iostream>
const double PI = 3.14159265358979323846;
double radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}
int main(){
    glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.20200683, 1.2202051, -0.06905545)) *
                       glm::mat4_cast(glm::quat(-0.244638, -0.35395727, 0.15726915, -0.8888942));
    float nearClip = 0.1f; // Near clipping plane distance
    float farClip = 1000.0f; // Far clipping plane distance
    glm::mat4 projectionMatrix = glm::perspectiveRH_ZO(glm::radians(radiansToDegrees(0.6981317) - radiansToDegrees(-0.94247776)),
                                                   glm::radians(radiansToDegrees(0.7330383) - radiansToDegrees(-0.94247776)),
                                                   static_cast<double>(nearClip), static_cast<double>(farClip));
    // glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0.16990215, -0.13591814, -0.50386375)) *
    //                    glm::mat4_cast(glm::quat(-0.14068037, -0.14248136, -0.001580268, 0.9797477)); local eye gaze
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0.17104617, -0.1370368, -0.50499034)) *
                       glm::mat4_cast(glm::quat(-0.14149708,-0.14095125, -0.00200063, 0.97985065));//global eye gaze
    glm::vec4 objectCoordinates(-0.14149708, -0.14095125, -0.00200063, 0.97985065);
    glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
    glm::vec4 clipSpaceCoordinates = mvpMatrix * objectCoordinates;
    glm::vec3 ndcCoordinates = glm::vec3(
    clipSpaceCoordinates.x / clipSpaceCoordinates.w,
    clipSpaceCoordinates.y / clipSpaceCoordinates.w,
    clipSpaceCoordinates.z / clipSpaceCoordinates.w
    );

    // Convert NDC coordinates to screen coordinates
    float screenWidth = 3712.0f;
    float screenHeight = 2016.0f;
    glm::vec2 screenCoordinates = glm::vec2(
        (ndcCoordinates.x + 1.0f) * 0.5f * screenWidth,
        (ndcCoordinates.y + 1.0f) * 0.5f * screenHeight
    );
    std::cout<<"x: "<<screenCoordinates.x<<"y: "<<screenCoordinates.y<<std::endl;
}