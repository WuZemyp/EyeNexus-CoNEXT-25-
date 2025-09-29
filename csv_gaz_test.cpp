

#include "C:\Users\13513\ALVR_Private\ALVR\eigen-3.4.0\Eigen\Dense"
#include <iostream>
#include <cmath>
#include <math.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
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
Eigen::Vector3d QuatToEuler(const Eigen::Quaterniond& quat) {
    Eigen::Vector3d euler = quat.toRotationMatrix().eulerAngles(2, 1, 0);
    return euler;
}
std::tuple<double, double, double, double> ComputeEyeGazeLocation(int frame_width,int frame_height,double yaw,double pitch,double left_view_fov_left,double left_view_fov_right,double left_view_fov_up,double left_view_fov_down,double right_view_fov_left,double right_view_fov_right,double right_view_fov_up,double right_view_fov_down){
    //separate for two frame
        // left frame left eye gaze
        bool left_yaw_positive=0;
        bool left_pitch_positive=0;
        double left_frame_x=0.0;
        double left_frame_y=0.0;
        if(yaw>0){
            left_yaw_positive=1;
        }
        if(pitch>0){
            left_pitch_positive=1;
        }

        if(left_yaw_positive){
            double projection_horizontal_left_frame = std::tan(std::abs(yaw)) / tan(std::abs(left_view_fov_left)) * (frame_width / 2);
            left_frame_x=frame_width/2-projection_horizontal_left_frame;
        }else{
            double projection_horizontal_left_frame = std::tan(std::abs(yaw)) / tan(std::abs(left_view_fov_right)) * (frame_width / 2);
            left_frame_x=frame_width/2+projection_horizontal_left_frame;

        }
        if(left_pitch_positive){
            double projection_vertical_left_frame = std::tan(std::abs(pitch)) / tan(std::abs(left_view_fov_up)) * (frame_height / 2);
            left_frame_y=frame_height/2+projection_vertical_left_frame;
            
        }else{
            double projection_vertical_left_frame = std::tan(std::abs(pitch)) / tan(std::abs(left_view_fov_down)) * (frame_height / 2);
            left_frame_y=frame_height/2-projection_vertical_left_frame;

        }
        // right frame right eye gaze
        bool right_yaw_positive=0;
        bool right_pitch_positive=0;
        double right_frame_x=0.0;
        double right_frame_y=0.0;
        if(yaw>0){
            right_yaw_positive=1;
        }
        if(pitch>0){
            right_pitch_positive=1;
        }
         if(right_yaw_positive){
            double projection_horizontal_right_frame = std::tan(std::abs(yaw)) / tan(std::abs(right_view_fov_left)) * (frame_width / 2);
            right_frame_x=frame_width/2-projection_horizontal_right_frame;
        }else{
            double projection_horizontal_right_frame = std::tan(std::abs(yaw)) / tan(std::abs(right_view_fov_right)) * (frame_width / 2);
            right_frame_x=frame_width/2+projection_horizontal_right_frame;

        }
        if(right_pitch_positive){
            double projection_vertical_right_frame = std::tan(std::abs(pitch)) / tan(std::abs(right_view_fov_up)) * (frame_height / 2);
            right_frame_y=frame_height/2+projection_vertical_right_frame;
            
        }else{
            double projection_vertical_right_frame = std::tan(std::abs(pitch)) / tan(std::abs(right_view_fov_down)) * (frame_height / 2);
            right_frame_y=frame_height/2-projection_vertical_right_frame;

        }
        //end for separate calculation
        return std::make_tuple(left_frame_x,left_frame_y,right_frame_x,right_frame_y);
}

int main()
{
    std::ifstream file("C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR\\eyegaze.csv"); 
    std::ofstream outputFile("projection_result.csv");
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
    // Example values
    for (const auto& row : data) {
        if(first==0){
            first++;
            outputFile<<"x"<<","<<"y"<<std::endl;
            continue;
        }
       
        Quaterniond eye_orientation_quat(std::stod(row[0]),std::stod(row[1]),std::stod(row[2]), std::stod(row[3])); // Eye orientation quaternion
        Eigen::Quaterniond quat(std::stod(row[0]),std::stod(row[1]),std::stod(row[2]), std::stod(row[3]));
        Eigen::Vector3d euler = QuatToEuler(quat);

        std::cout << "Euler angles (roll, pitch, yaw): " << euler[2] << ", " << euler[1] << ", " << euler[0] << std::endl;
        Vector3d eye_position(std::stod(row[4]), std::stod(row[5]),std::stod(row[6])); // Eye position
        //Vector3d head_position(0.0, 0.0, 0.0); // Head position
        double left_fov_up = radiansToDegrees(std::abs(std::stod(row[21]))); // Left eye field of view: up angle
        double left_fov_down = radiansToDegrees(std::abs(std::stod(row[22]))); // Left eye field of view: down angle
        double left_fov_left = radiansToDegrees(std::abs(std::stod(row[23]))); // Left eye field of view: left angle
        double left_fov_right = radiansToDegrees(std::abs(std::stod(row[24]))); // Left eye field of view: right angle
        // double right_fov_up = radiansToDegrees(std::stod(row[32])); // Right eye field of view: up angle
        // double right_fov_down = radiansToDegrees(std::stod(row[33])); // Right eye field of view: down angle
        // double right_fov_left = radiansToDegrees(std::stod(row[34])); // Right eye field of view: left angle
        // double right_fov_right = radiansToDegrees(std::stod(row[35])); // Right eye field of view: right angle
        double fov_x=left_fov_up+left_fov_down;
        double fov_y=left_fov_left+left_fov_right;
        

        int frame_width = 3712; // Frame width in pixels
        int frame_height = 2016; // Frame height in pixels






        //separate for two frame
        // left frame left eye gaze
        bool left_yaw_positive=0;
        bool left_pitch_positive=0;
        double left_frame_x=0.0;
        double left_frame_y=0.0;
        if(std::stod(row[36])>0){
            left_yaw_positive=1;
        }
        if(std::stod(row[37])>0){
            left_pitch_positive=1;
        }

        if(left_yaw_positive){
            double projection_horizontal_left_frame = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[23]))) * (frame_width / 2);
            left_frame_x=frame_width/2-projection_horizontal_left_frame;
        }else{
            double projection_horizontal_left_frame = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[24]))) * (frame_width / 2);
            left_frame_x=frame_width/2+projection_horizontal_left_frame;

        }
        if(left_pitch_positive){
            double projection_vertical_left_frame = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[21]))) * (frame_height / 2);
            left_frame_y=frame_height/2+projection_vertical_left_frame;
            
        }else{
            double projection_vertical_left_frame = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[22]))) * (frame_height / 2);
            left_frame_y=frame_height/2-projection_vertical_left_frame;

        }
        // right frame right eye gaze
        bool right_yaw_positive=0;
        bool right_pitch_positive=0;
        double right_frame_x=0.0;
        double right_frame_y=0.0;
        if(std::stod(row[36])>0){
            right_yaw_positive=1;
        }
        if(std::stod(row[37])>0){
            right_pitch_positive=1;
        }
         if(right_yaw_positive){
            double projection_horizontal_right_frame = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[34]))) * (frame_width / 2);
            right_frame_x=frame_width/2-projection_horizontal_right_frame;
        }else{
            double projection_horizontal_right_frame = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[35]))) * (frame_width / 2);
            right_frame_x=frame_width/2+projection_horizontal_right_frame;

        }
        if(right_pitch_positive){
            double projection_vertical_right_frame = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[32]))) * (frame_height / 2);
            right_frame_y=frame_height/2+projection_vertical_right_frame;
            
        }else{
            double projection_vertical_right_frame = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[33]))) * (frame_height / 2);
            right_frame_y=frame_height/2-projection_vertical_right_frame;

        }
        //end for separate calculation

        double projection_horizontal_left = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[23]))) * (frame_width / 2);
        double projection_horizontal_right = std::tan(std::abs(std::stod(row[36]))) / tan(std::abs(std::stod(row[24]))) * (frame_width / 2);
        

        double projection_vertical_up = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[21]))) * (frame_height / 2);
        double projection_vertical_down = std::tan(std::abs(std::stod(row[37]))) / tan(std::abs(std::stod(row[22]))) * (frame_height / 2);

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
        outputFile << (frame_width/2 - projection_horizontal_left) << "," << (frame_height/2 + projection_vertical_up)<< "," <<projection_horizontal_left<< "," <<projection_horizontal_right<< "," <<projection_vertical_up<< "," <<projection_vertical_down<< "," <<left_frame_x<< "," <<left_frame_y<< "," <<right_frame_x<< "," <<right_frame_y;
        outputFile << std::endl;
        // Print the duration
        std::cout << "Time interval: " << duration << " milliseconds." << std::endl;
    }
    file.close();
    outputFile.close();
    return 0;
}

