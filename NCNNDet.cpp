#include "NCNNDet.h"

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



static float Get(const ncnn::Mat& matrix, int width, int height, int channel = 0){
    assert(channel < matrix.c);
    assert(width < matrix.w);
    assert(height < matrix.h);


    const float* ptr = matrix.channel(channel);
    return ptr[width + matrix.w * height];
}







NCNNDet::NCNNDet(const char* paramFile, const char* binFile,int width, int height):
_width(width),_height(height)
//Simple trick to get the variable as class member without using smart pointers oder oder STL
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

std::vector<ncnn::Mat> NCNNDet::GetData(cv::Mat frame){

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
        assert(smallFrame.cols <= 320);
        assert(smallFrame.rows <= 320);
        if(smallFrame.cols < 320){
            left = (320 - smallFrame.cols)/2;
            right = (320 - smallFrame.cols)/2 + (320 - smallFrame.cols)%2;
        }
        if(smallFrame.rows < 320){
            top = (320 - smallFrame.rows)/2;
            bottom = (320 - smallFrame.rows)/2 + (320 - smallFrame.rows)%2;
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
    
    
    ncnn::Mat out0, out1, out2, out3, out4, out5;  
    //Get the result of the forward pass
    if(ex.extract("out0", out0) != 0){
        std::cerr<<"Could not extract out0\n";
    }
    if(ex.extract("out1", out1) != 0){
        std::cerr<<"Could not extract out1\n";
    }
    if(ex.extract("out2", out2) != 0){
        std::cerr<<"Could not extract out2\n";
    }
    if(ex.extract("out3", out3) != 0){
        std::cerr<<"Could not extract out3\n";
    }
    if(ex.extract("out4", out4) != 0){
        std::cerr<<"Could not extract out4\n";
    }
    if(ex.extract("out5", out5) != 0){
        std::cerr<<"Could not extract out5\n";
    }

    //The first three output (out0, out1, out2) represet the logits result of an object centered at that location.
    //We assume the other three outputs (out3, out4, out5) represent the distance from the grid center to sides of the bbox left,top, right,bottom
    return {out0,out3, out1, out4,out2, out5};
}




static BBox Idx2BBox(const ncnn::Mat& matrix, int x, int y, float stride, float width, float height){
    
    //For some reason, probably due to the conversion process, the deltas are in pixel space of the input image.
    

    //Convert the deltas to bounding boxes in normalized coordinates
    BBox top;
    top._lt.x = ((float)x + 0.5f) * (float)stride - Get(matrix,x, y, 0) / (float)width; 
    top._lt.y = ((float)y + 0.5f) * (float)stride - Get(matrix,x, y, 1) / (float)height; 
    top._br.x = ((float)x + 0.5f) * (float)stride + Get(matrix,x, y, 2) / (float)width;  
    top._br.y = ((float)y + 0.5f) * (float)stride + Get(matrix,x, y, 3) / (float)height;
    return top;
}


std::vector<BBox> NCNNDet::GetBBoxes(const std::vector<ncnn::Mat> &tensors, const float& threshold){
    //Get all Boxes from the layers. It is assumed, that always will be [cls, reg_box, cls, ...]

    std::vector<BBox> boxes;
    //Iterate through each layer, Each layers is composed from one classification and one regression
    for(int i = 0; i < tensors.size(); i+= 2){

        //Calculate the stride of the layer The stride should be equal for both
        float stride = 1.0f / tensors[i].w;
        for (int y = 0; y < tensors[i].h; ++y) {
            for (int x= 0; x < tensors[i].w; ++x) {
                //For each region, get the probability of a object
                float prob = sigmoid(Get(tensors[i], x, y));
                //Discard if below threshold
                if(prob >= threshold){
                    //Convert the deltas to a Bounding Box
                    BBox obj = Idx2BBox(tensors[i+1],x, y, stride, _width, _height);
                    boxes.push_back(obj);
                }
            }
        }
    }
    
    
    return boxes;
}