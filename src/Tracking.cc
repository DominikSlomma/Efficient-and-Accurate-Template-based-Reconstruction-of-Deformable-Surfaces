#include "Tracking.h"
#include "Extractor.h"
#include "MeshMap.h"
#include <iostream>
#include <algorithm>
#include<fstream>

Tracking::Tracking(cv::Mat &frame, Eigen::Matrix3d K, std::vector<Eigen::Vector3d> &vertices, std::vector<Eigen::Vector3i> &triangles, int thresholdValue) :  K_(K), fx_(K(0,0)), fy_(K(1,1)), cx_(K(0,2)), cy_(K(1,2)), pre_frame_(frame),
    vertices_(vertices), triangles_(triangles), number_triangles_(triangles_.size()), number_vertices_(vertices_.size())
{   
    
    createMask(thresholdValue);

    findUsableVerticies();
    
    findUsableTriangles();
    
    createInitialObseration();
    extraction = new Extractor(frame, pixel_reference_);
}

void Tracking::setunordered_mapping(MeshMap *unordered_map) {
    unordered_map_ = unordered_map;
    unordered_map_->set_Observation(obs);
}

std::vector<double> Tracking::getObservation() {
    return obs;
}


void Tracking::createInitialObseration() {
    
    bool used_vertex[vertices_.size()];
    for (size_t i = 0; i < vertices_.size(); ++i) {
        used_vertex[i] = false; // oder 0, falls das Array Werte vom Typ char enthält
    }
    for(int face_id=0;face_id < triangles_.size(); face_id++) {
        
        if(!usable_triangles_[face_id])
            continue;

        int f1 = triangles_[face_id].x();
        int f2 = triangles_[face_id].y();
        int f3 = triangles_[face_id].z();
        
        for(int id=0; id<4;id++) {


            double alpha = alp[id];
            double beta = bet[id];
            double gamma = gam[id];
            
            if(int(alpha) == 1) {
                if(used_vertex[f1]) {
                    continue;
                } else {
                    used_vertex[f1] = true;
                }
            }
            if(int(beta) == 1) {
                if(used_vertex[f2]) {
                    continue;
                } else {
                    used_vertex[f2] = true;
                }
            }
            if(int(gamma) == 1) {
                if(used_vertex[f3]) {
                    continue;
                } else {
                    used_vertex[f3] = true;
                }
            }
            if ((alpha +beta+gamma >1) || (alpha +beta+gamma < 0)){
                std::cerr << "alpha, beta and gamma wrong!" << std::endl;
                exit(-1);
            }

            Eigen::Vector3d p = vertices_[f1] * alpha + vertices_[f2] * beta + vertices_[f3] * gamma; 

            double x = p.x();
            double y = p.y();
            double z = p.z();

            double u = fx_*x/z + cx_;
            double v = fy_*y/z + cy_;

            obs.push_back(face_id);
            obs.push_back(u);
            obs.push_back(v);
            obs.push_back(alpha);
            obs.push_back(beta);
            obs.push_back(gamma);
        //     std::cout << face_id << " " << u << " " << v << " " 
        // << alpha << " " << beta << " " << gamma << " " << std::endl;
            pixel_reference_.push_back(cv::Point2f((u),(v)));
        }
    }
}

void Tracking::getObs(std::vector<double> &observation) {
    observation = obs;
}


void Tracking::findUsableVerticies() {
    for(int i=0; i < number_vertices_; i++) {
        bool isPointUsable = false;
        double x = vertices_[i].x();
        double y = vertices_[i].y();
        double z = vertices_[i].z();
        
        double u = fx_*x/z + cx_;
        double v = fy_*y/z + cy_;
        if((u >= 2) && (v >= 5) && (u < 360) && (v < 288) && (mask_.at<uchar>(int(v),int(u)) == 255)) {
            isPointUsable = true;
        } 
        usable_vertices_.push_back(isPointUsable);
    }
}

void Tracking::findUsableTriangles() {

    for(int i=0;i < triangles_.size();i++) {
        bool isTriangleUsabel = false;
        int f1 = triangles_[i].x();
        int f2 = triangles_[i].y();
        int f3 = triangles_[i].z();

        if(usable_vertices_[f1] && usable_vertices_[f2] && usable_vertices_[f3]) {
            isTriangleUsabel = true;
        }
        usable_triangles_.push_back(isTriangleUsabel);
    }
}


void Tracking::createMask(int thresholdValue) {
    cv::Mat hsvImage;
    cv::cvtColor(pre_frame_, hsvImage, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsvImage, channels);
    cv::threshold(channels[2], mask_, thresholdValue, 255, cv::THRESH_BINARY);
}

void Tracking::draw_correspondence(cv::Mat &frame) {
    for(uint i = 0; i < pixel_reference_.size(); i++)
    {   if (extraction->status[i] == 1) {
            cv::line(frame, pixel_correspondence_[i], pixel_reference_[i], cv::Scalar(0, 255, 0), 1);
            cv::circle(frame, pixel_correspondence_[i], 1, cv::Scalar(0, 0, 255), -1);
        }
    }
}

#include <fstream>
#include <sstream>
#include <iostream>
void Tracking::updateObservation() {

    for(int obs_id=0; obs_id < obs.size()/6; obs_id++) {

        double u = pixel_correspondence_[obs_id].x;
        double v = pixel_correspondence_[obs_id].y;

        obs[obs_id*6+1] = u;
        obs[obs_id*6+2] = v;
    }
static int FrameNo = 0;
    // std::ofstream file("obs_test/obs_" + std::to_string(FrameNo) + ".txt");
    // FrameNo++;
    // for(int i=0; i < obs.size()/6; i++) {
    //     file <<  std::setprecision(20) << std::fixed <<  obs[i*6] << " " << obs[i*6+1] << " " << obs[i*6+2] << " " << obs[i*6+3] << " " << obs[i*6+4] << " " << obs[i*6+5] << std::endl;
    // }
    // file.close();
    
    std::string file_path = "obs_test3/obs_"+ std::to_string(FrameNo) +".txt";
    std::ifstream obj_file(file_path);
    if (!obj_file.is_open()) {
        std::cerr << "Incorrect path to the Obj-file." << std::endl;
        return;
    }
    std::string line;
    std::vector<double> tmp;
    while (std::getline(obj_file, line)) {
        std::stringstream sstream(line);
        std::string word;
        sstream >> word;
        tmp.push_back(std::stod(word));
        sstream >> word;
        tmp.push_back(std::stod(word));
        sstream >> word;
        tmp.push_back(std::stod(word));
        sstream >> word;
        tmp.push_back(std::stod(word));
        sstream >> word;
        tmp.push_back(std::stod(word));
        sstream >> word;
        tmp.push_back(std::stod(word));
    }
    obs = tmp;
    FrameNo++;
}


void Tracking::track(cv::Mat &frame, std::vector<cv::Point2f> &pixel) {
    cv::Mat modifiedFrame = frame.clone();

    extraction->extract(frame, pixel_correspondence_);
    updateObservation();

    this->draw_correspondence(modifiedFrame);

    cv::imshow("Frame", modifiedFrame);
    pixel = pixel_correspondence_;
    
    
    
    // static int FrameNo = 0;
    // std::ofstream file("obs_test3/obs_" + std::to_string(FrameNo) + ".txt");
    // FrameNo++;
    // for(int i=0; i < obs.size()/6; i++) {
    //     file <<  std::setprecision(20) << std::fixed <<  obs[i*6] << " " << obs[i*6+1] << " " << obs[i*6+2] << " " << obs[i*6+3] << " " << obs[i*6+4] << " " << obs[i*6+5] << std::endl;
    // }
    // file.close();

    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // if(FrameNo == 39)
    //     exit(1);
    // cv::waitKey(0);

}