#include "C:\Users\13513\ALVR_Private\ALVR\eigen-3.4.0\Eigen\Dense"
#include <iostream>
#include <cmath>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
 // You'll need to install the Eigen library for matrix operations

using namespace Eigen;
const double PI = 3.14159265358979323846;
double radiansToDegrees(double radians) {
    return radians * 180.0 / PI;
}

// Function to convert quaternion to rotation matrix
Matrix3d quaternionToRotationMatrix(const Quaterniond& q)
{
    Matrix3d rotation_matrix = q.normalized().toRotationMatrix();
    return rotation_matrix;
}

// Function to calculate the projection matrix
Matrix4d calculateProjectionMatrix(const double fov_up, const double fov_down, const double fov_left, const double fov_right, const int frame_width, const int frame_height)
{
    Matrix4d projection_matrix = Matrix4d::Zero();
    projection_matrix(0, 0) = 2.0 / (fov_right - fov_left);
    projection_matrix(1, 1) = 2.0 / (fov_up - fov_down);
    projection_matrix(0, 2) = (fov_right + fov_left) / (fov_right - fov_left);
    projection_matrix(1, 2) = (fov_up + fov_down) / (fov_up - fov_down);
    projection_matrix(2, 2) = -1.0;
    projection_matrix(2, 3) = -1.0;
    return projection_matrix;
}

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
    std::ifstream file("C:\\Users\\13513\\Desktop\\eyegaze.csv"); 

    if (!file) {
        std::cout << "can not open" << std::endl;
        return 1;
    }

    std::vector<std::vector<std::string>> data; 

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> row; // 保存每一行的数据

        std::stringstream ss(line);
        std::string token;

        while (std::getline(ss, token, ',')) {
            row.push_back(token);
        }

        data.push_back(row);
    }

    
    int first = 0; 

    for (const auto& row : data) {
        if(first==0){
            first++;
            continue;
        }
        Quaterniond eye_orientation_quat(std::stod(row[0]),std::stod(row[1]),std::stod(row[2]), std::stod(row[3])); // Eye orientation quaternion
        Vector3d eye_position(std::stod(row[4]), std::stod(row[5]),std::stod(row[6])); // Eye position
        //Vector3d head_position(0.0, 0.0, 0.0); // Head position
        double left_fov_up = radiansToDegrees(std::stod(row[21])); // Left eye field of view: up angle
        double left_fov_down = radiansToDegrees(std::stod(row[22])); // Left eye field of view: down angle
        double left_fov_left = radiansToDegrees(std::stod(row[23])); // Left eye field of view: left angle
        double left_fov_right = radiansToDegrees(std::stod(row[24])); // Left eye field of view: right angle
        double right_fov_up = radiansToDegrees(std::stod(row[32])); // Right eye field of view: up angle
        double right_fov_down = radiansToDegrees(std::stod(row[33])); // Right eye field of view: down angle
        double right_fov_left = radiansToDegrees(std::stod(row[34])); // Right eye field of view: left angle
        double right_fov_right = radiansToDegrees(std::stod(row[35])); // Right eye field of view: right angle
        int frame_width = 3712; // Frame width in pixels
        int frame_height = 2016; // Frame height in pixels

        // Step 1: Convert eye's quaternion to rotation matrix
        Matrix3d eye_rotation_matrix = quaternionToRotationMatrix(eye_orientation_quat);

        // Step 2: Transform eye's position to global position
        Vector3d global_eye_position = eye_position;

        // Step 3: Calculate projection matrix for each eye
        Matrix4d left_eye_projection_matrix = calculateProjectionMatrix(left_fov_up, left_fov_down, left_fov_left, left_fov_right, frame_width, frame_height);
        Matrix4d right_eye_projection_matrix = calculateProjectionMatrix(right_fov_up, right_fov_down, right_fov_left, right_fov_right, frame_width, frame_height);
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
    }
    

    return 0;
}