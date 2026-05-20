#include "NCNNPose.h"

#include "mat.h"
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include "iostream"
#include "fstream"
#include "utils.h"



static float Get(const ncnn::Mat& matrix, int width, int height, int channel = 0){
    assert(channel < matrix.c);
    assert(width < matrix.w);
    assert(height < matrix.h);


    const float* ptr = matrix.channel(channel);
    return ptr[width + matrix.w * height];
}




// Helper function to load raw float binaries
std::vector<float> load_binary(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open weight file: " << path << std::endl;
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<float> buffer(size / sizeof(float));
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}


NCNNPose::NCNNPose(const char* paramFile, const char* binFile,int width, int height):
_width(width),_height(height)
{
    //Set the use vulkan to false, since the RPI5 GPU is not useful for GPGPU
    _net.opt.use_vulkan_compute = false; 
    
    if(!std::filesystem::exists(paramFile)){
        std::cerr<<"Param File: " << paramFile << "does not exist\n";
        exit(EXIT_FAILURE);
    }
    //Load the structure of the network
    _net.load_param(paramFile);


    if(!std::filesystem::exists(binFile)){
        std::cerr<<"Bin (weights) File: " << binFile << "does not exist\n";
        exit(EXIT_FAILURE);
    }
    //Load the weights of the network    
    _net.load_model(binFile);
}


static float sigmoid(float z){

    return 1.0f / (1.0f + std::exp(-z));
}

std::vector<Keypoint> NCNNPose::GetData(cv::Mat frame){

    // Prepare input data
    cv::Mat readyFrame;
    {
        //Resize the input frame to respect our given sizes
        float scale_x =  (float)_width / (float)frame.cols;
        float scale_y =  (float)_height / (float)frame.rows; 
        float scale = std::min(scale_x, scale_y);
        cv::Mat smallFrame;
        cv::resize(frame, smallFrame, cv::Size(), scale, scale, cv::INTER_AREA);

        int top = 0, bottom = 0, left = 0, right = 0;
        assert(smallFrame.cols <= _width);
        assert(smallFrame.rows <= _height);
        if(smallFrame.cols < _width){
            left = (_width - smallFrame.cols)/2;
            right = (_width - smallFrame.cols)/2 + (_width - smallFrame.cols)%2;
        }
        if(smallFrame.rows < _height){
            top = (_height - smallFrame.rows)/2;
            bottom = (_height - smallFrame.rows)/2 + (_height - smallFrame.rows)%2;
        }
        
        cv::copyMakeBorder(smallFrame, readyFrame, top, bottom, left, right, cv::BORDER_DEFAULT);
    }
    
    ncnn::Mat in = ncnn::Mat::from_pixels(readyFrame.data, ncnn::Mat::PIXEL_BGR2RGB, readyFrame.cols, readyFrame.rows);
    
    //Normalization values
    const float mean_vals[3] = { 123.675f, 116.28f,103.53f};
    const float norm_vals[3] = { 1.0f / 58.395f, 1.0f / 57.12f,1.0f / 57.375f};

    //Normalize
    in.substract_mean_normalize(mean_vals, norm_vals);

    //Create the extractor
    ncnn::Extractor ex = _net.create_extractor();
    //Do a forward pass
    ex.input("in0", in);
    
    
    ncnn::Mat out0, out1;  
    //Get the result of the forward pass
    if(ex.extract("out0", out0) != 0){
        std::cerr<<"Could not extract out0\n";
    }
    //Get the result of the forward pass
    if(ex.extract("out1", out1) != 0){
        std::cerr<<"Could not extract out0\n";
    }
   

    //Process all the SimCC Coordinates
    std::vector<Keypoint> keypoints(out0.h);

    for()



    return std::vector<Keypoint>();
}