

#include "C:\Users\13513\ALVR_Private\ALVR\eigen-3.4.0\Eigen\Dense"
#include <iostream>
#include <cmath>
#include <math.h>
#include <chrono>

using namespace Eigen;
const double PI = 3.14159265358979323846;
double radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}
Eigen::Matrix4d calculateProjectionMatrix(double fov_x, double fov_y, int frame_width, int frame_height, double near, double far) {
    // Convert field of view angles to radians
    double fov_x_rad = fov_x * PI / 180.0;
    double fov_y_rad = fov_y * PI / 180.0;

    // Calculate aspect ratio
    double aspect_ratio = static_cast<double>(frame_width) / frame_height;

    // Calculate parameters for constructing the projection matrix
    double right = near * std::tan(fov_x_rad / 2);
    double left = -right;
    double top = near * std::tan(fov_y_rad / 2);
    double bottom = -top;

    // Construct the projection matrix
    Eigen::Matrix4d projection_matrix;
    projection_matrix << (2 * near) / (right - left), 0, (right + left) / (right - left), 0,
                         0, (2 * near) / (top - bottom), (top + bottom) / (top - bottom), 0,
                         0, 0, -(far + near) / (far - near), -(2 * far * near) / (far - near),
                         0, 0, -1, 0;

    return projection_matrix;
}
// Function to convert quaternion to rotation matrix
Matrix3d quaternionToRotationMatrix(const Quaterniond& q)
{
    Matrix3d rotation_matrix = q.normalized().toRotationMatrix();
    return rotation_matrix;
}

// Function to calculate the projection matrix


// Function to convert NDC coordinates to pixel coordinates
Vector2f convertNdcToPixelCoordinates(const Vector2f& ndc_coordinates, const int frame_width, const int frame_height)
{
    Vector2f pixel_coordinates;
    pixel_coordinates[0] = (ndc_coordinates[0] + 1.0) * 0.5 * frame_width;
    pixel_coordinates[1] = (1.0 - ndc_coordinates[1]) * 0.5 * frame_height;
    return pixel_coordinates;
}

// Function to interpolate eye gaze point for higher resolution
Vector2f interpolateEyeGazePoint(const Vector2f& left_eye_pixel_coordinates, const Vector2f& right_eye_pixel_coordinates)
{
    Vector2f interpolated_gaze_point = (left_eye_pixel_coordinates + right_eye_pixel_coordinates) * 0.5;
    return interpolated_gaze_point;
}

int main()
{
    // Example values
    Quaterniond eye_orientation_quat(-0.14068037, -0.14248136, -0.001580268, 0.9797477); // Eye orientation quaternion
    Vector3d eye_position(-0.16990215, -0.13591814,-0.50386375); // Eye position
    //Vector3d head_position(0.0, 0.0, 0.0); // Head position
    double left_fov_up = radiansToDegrees(0.7330383); // Left eye field of view: up angle
    double left_fov_down = radiansToDegrees(0.94247776); // Left eye field of view: down angle
    double left_fov_left = radiansToDegrees(0.94247776); // Left eye field of view: left angle
    double left_fov_right = radiansToDegrees(0.6981317); // Left eye field of view: right angle
    double right_fov_up = radiansToDegrees(0.7330383); // Right eye field of view: up angle
    double right_fov_down = radiansToDegrees(-0.94247776); // Right eye field of view: down angle
    double right_fov_left = radiansToDegrees(-0.94247776); // Right eye field of view: left angle
    double right_fov_right = radiansToDegrees(0.6981317); // Right eye field of view: right angle
    double fov_x=left_fov_up+left_fov_down;
    double fov_y=left_fov_left+left_fov_right;
    std::cout<<fov_x<<std::endl;
    std::cout<<fov_y<<std::endl;

    int frame_width = 3712; // Frame width in pixels
    int frame_height = 2016; // Frame height in pixels

    // Step 1: Convert eye's quaternion to rotation matrix
    auto start = std::chrono::high_resolution_clock::now();
    Matrix3d eye_rotation_matrix = quaternionToRotationMatrix(eye_orientation_quat);

    // Step 2: Transform eye's position to global position
    Vector3d global_eye_position = eye_position;


    

    // Step 3: Calculate projection matrix for each eye
    Matrix4d left_eye_projection_matrix = calculateProjectionMatrix(fov_x,fov_y,frame_width,frame_height,0.1f,1000.0f);
    Matrix4d right_eye_projection_matrix = calculateProjectionMatrix(fov_x,fov_y,frame_width,frame_height,0.1f,1000.0f);
    std::cout<<"left eye projection matrix:"<<std::endl<<left_eye_projection_matrix<<std::endl;
    std::cout<<"left eye position vector:"<<std::endl<<Vector4d(global_eye_position[0], global_eye_position[1], global_eye_position[2], 1.0)<<std::endl;
    // Step 4: Apply projection matrix to eye's global position
    Vector4d left_eye_ndc = left_eye_projection_matrix * Vector4d(global_eye_position[0], global_eye_position[1], global_eye_position[2], 1.0);
    Vector4d right_eye_ndc = right_eye_projection_matrix * Vector4d(global_eye_position[0], global_eye_position[1], global_eye_position[2], 1.0);
    std::cout << " left ndc coordinates: (" << left_eye_ndc[0] << ", " << left_eye_ndc[1] <<","<< left_eye_ndc[2]<<","<<left_eye_ndc[3]<< ")" << std::endl;
    std::cout << " right ndc coordinates: (" << right_eye_ndc[0] << ", " << right_eye_ndc[1] <<","<< right_eye_ndc[2]<<","<<right_eye_ndc[3]<< ")" << std::endl;


    // Step 5: Convert NDC coordinates to pixel coordinates
    Vector2f left_eye_pixel_coordinates = convertNdcToPixelCoordinates(Vector2f(left_eye_ndc[0] / left_eye_ndc[3], left_eye_ndc[1] / left_eye_ndc[3]), frame_width, frame_height);
    Vector2f right_eye_pixel_coordinates = convertNdcToPixelCoordinates(Vector2f(right_eye_ndc[0] / right_eye_ndc[3], right_eye_ndc[1] / right_eye_ndc[3]), frame_width, frame_height);
    std::cout << " left pixel coordinates: (" << left_eye_pixel_coordinates[0] << ", " << left_eye_pixel_coordinates[1] << ")" << std::endl;
    std::cout << " right pixel coordinates: (" << right_eye_pixel_coordinates[0] << ", " << right_eye_pixel_coordinates[1] << ")" << std::endl;


    // Step 6: Interpolate eye gaze point
    Vector2f interpolated_gaze_point = interpolateEyeGazePoint(left_eye_pixel_coordinates, right_eye_pixel_coordinates);

    // Step 7: Display the interpolated gaze point
    std::cout << "Interpolated Gaze Point: (" << interpolated_gaze_point[0] << ", " << interpolated_gaze_point[1] << ")" << std::endl;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Print the duration
    std::cout << "Time interval: " << duration << " milliseconds." << std::endl;
    return 0;
}
