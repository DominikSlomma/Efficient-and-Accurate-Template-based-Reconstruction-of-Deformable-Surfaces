// #include "MeshMap.h"
// #include "Tracking.h"
// #include "viewer/Mesh_Visualizer.h"
#include "System.h"
#include "GT_compare/SfTGT.h"
#include "utils.h"
#include <opencv2/opencv.hpp>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>
#include <cmath>
#include <sstream>

#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace stbr;

int main() {
    // frame_id
   

    // Config
    const YAML::Node config = YAML::LoadFile("../app/p_sft_S7_real/config.yaml");
    
    // Todo
    SfTGT* gt = new SfTGT(config);
    // Creation of a mesh
    std::string img_file_path = config["System"]["video_file_path"].as<std::string>();
    // std::string obj_file_path = config["System"]["reference_file_path"].as<std::string>();


    cv::Mat frame;
    // cv::VideoCapture cap(video_file);
    // cap >> frame;
    std::shared_ptr<open3d::geometry::TriangleMesh> mesh;
    // mesh = createRefMesh(config, gt_id);
    std::vector<Eigen::Vector3d> vertices;
    std::vector<Eigen::Vector3i> triangles;
    utils::getMesh(config["System"]["reference_file_path"].as<std::string>(), vertices, triangles);
    
    // auto test = std::make_shared<open3d::geometry::PointCloud>();
    // open3d::io::ReadPointCloud("/home/anonym/Schreibtisch/PhD/code/Sparse Template based Reconstruction/data/phi_SfT/real/S1/point_clouds/point_cloud_000.ply", *test);
    // auto point_cloud_ptr = open3d::io::CreatePointCloudFromFile("/home/anonym/Schreibtisch/PhD/code/Sparse Template based Reconstruction/data/phi_SfT/real/S1/point_clouds/point_cloud_000.ply");
    // std::cout << test->points_.size() << std::endl;
    // open3d::visualization::DrawGeometries({test});
    // std::shared_ptr<open3d::geometry::TriangleMesh> mesh_temp;
    // mesh_temp->vertices_ = vertices;
    // mesh_temp->triangles_ = triangles;
    // open3d::visualization::DrawGeometries({mesh_temp});

    frame = cv::imread(img_file_path + "000.png", cv::IMREAD_COLOR);
    // cv::rotate(frame, frame, cv::ROTATE_180);
    System *sys = new System(triangles, vertices, frame, config, gt); 

    int max_number = config["Phi_SfT"]["max_number_frames"].as<int>();

    bool end = false;
    bool only_once = config["Phi_SfT"]["only_once"].as<bool>();

    while(!end){
        frame = cv::imread(img_file_path + "000.png", cv::IMREAD_COLOR);
        for (int num_img=1;num_img < max_number; num_img++) {
            
            
            sys->monocular_feed(frame);
            std::stringstream ss;
            ss << std::setw(3) << std::setfill('0') << num_img;
            std::string result = ss.str();
            frame = cv::imread(img_file_path + result + ".png", cv::IMREAD_COLOR);
            // cv::rotate(frame, frame, cv::ROTATE_180);

            int key = cv::waitKey(1);
            if (key == 'q')
            {
                std::cout << "q key is pressed by the user. Stopping the video" << std::endl;
                end = true;
                break;
            }
            
            
        }

        if(only_once)
            break;
        
    }
    
    double sum = 0;
    for (double element : gt->all_mean_) {
        sum += element;
    }
    double average = static_cast<double>(sum) / gt->all_mean_.size();
    std::cout << average << std::endl;
    

    return 0;
}

