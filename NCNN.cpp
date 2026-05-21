#include "NCNN.h"

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
#include "net.h"
#include "utils.h"


static float Get(const ncnn::Mat& matrix, int width, int height, int channel = 0){
    assert(channel < matrix.c);
    assert(width < matrix.w);
    assert(height < matrix.h);


    const float* ptr = matrix.channel(channel);
    return ptr[width + matrix.w * height];
}







NCNN::NCNN(const char* paramFile, const char* binFile,int width, int height):
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

ncnn::Extractor NCNN::PrepareInput(const cv::Mat& frame){
    // Prepare input data
    cv::Mat readyFrame;
    {
        //Resize the input frame to respect our given sizes
        float scale_x =  (float)_width / (float)frame.cols;
        float scale_y =  (float)_height / (float)frame.rows; 
        float scale = std::min(scale_x, scale_y);
        cv::Mat smallFrame;
        cv::resize(frame, smallFrame, cv::Size(), scale, scale, cv::INTER_AREA);

        
        assert(smallFrame.cols <= _width);
        assert(smallFrame.rows <= _height);
        int top = 0, bottom = 0, left = 0, right = 0;
        if(smallFrame.cols < _width){
            left = (_width - smallFrame.cols)/2;
            right = (_width - smallFrame.cols)/2 + (_width - smallFrame.cols)%2;
            _rescale_x = (float)_width/ ((float)_width - (float)(left + right));
            _shift_x = (float)left/(float)_width;
        } else {
            _rescale_x = 1.0f;
            _shift_x = 0.0f;
        }
        if(smallFrame.rows < _height){
            top = (_height - smallFrame.rows)/2;
            bottom = (_height - smallFrame.rows)/2 + (_height - smallFrame.rows)%2;
            _rescale_y = (float)_height/ ((float)_height - (float)(top + bottom));
            _shift_y = (float)top/(float)_height;
        }else {
            _rescale_y = 1.0f;
            _shift_y = 0.0f;
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

    return ex;
}





static float sigmoid(float z){

    return 1.0f / (1.0f + std::exp(-z));
}

BBox NCNNDet::Idx2BBox(const ncnn::Mat& matrix, int x, int y, float stride){
    
    //For some reason, probably due to the conversion process, the deltas are in pixel space of the input image.
    

    //Convert the deltas to bounding boxes in normalized coordinates
    BBox top;
    top._lt.x = (((float)x) * (float)stride - Get(matrix,x, y, 0) / (float)_width    - _shift_x)*_rescale_x; 
    top._lt.y = (((float)y) * (float)stride - Get(matrix,x, y, 1) / (float)_height   - _shift_y)*_rescale_y; 
    top._br.x = (((float)x) * (float)stride + Get(matrix,x, y, 2) / (float)_width    - _shift_x)*_rescale_x;
    top._br.y = (((float)y) * (float)stride + Get(matrix,x, y, 3) / (float)_height   - _shift_y)*_rescale_y;
    return top;
}

std::vector<BBox> NCNNDet::GetData(ncnn::Extractor ex,const float& threshold){
    
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
    std::vector<ncnn::Mat> tensors {out0,out3, out1, out4,out2, out5};

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
                    BBox obj = Idx2BBox(tensors[i+1],x, y, stride);
                    boxes.push_back(obj);
                }
            }
        }
    }
    
    
    return boxes;
}












std::vector<Keypoint> NCNNPose::GetData(ncnn::Extractor ex){
    
    ncnn::Mat out0, out1, out2, out3, out4, out5;  
    //Get the result of the forward pass
    if(ex.extract("out0", out0) != 0){
        std::cerr<<"Could not extract out0\n";
    }
    if(ex.extract("out1", out1) != 0){
        std::cerr<<"Could not extract out1\n";
    }

    //Process the x points
    //The height component defines the amount of keypoints
    std::vector<Keypoint> keypoints(out0.h, Keypoint{0,0,0,0});

    for(int feat = 0; feat < out0.h; ++feat){
        Keypoint& feature = keypoints[feat];
        //Iterate through all possible coordinates
        for(int coord = 0; coord < out0.w; ++coord){
            if(float currProb = Get(out0, coord, feat); currProb > feature.prob_x){
                feature.x = coord;
                feature.prob_x = currProb;
            }
        }
        //Normalize the value of each coordinate.
        feature.x = ((float)feature.x / (float)out0.w - _shift_x)*_rescale_x;
    }
    for(int feat = 0; feat < out1.h; ++feat){
        Keypoint& feature = keypoints[feat];
        //Iterate through all possible coordinates
        for(int coord = 0; coord < out1.w; ++coord){
            if(float currProb = Get(out1, coord, feat); currProb > feature.prob_y){
                feature.y = coord;
                feature.prob_y = currProb;
            }
        }
        //Normalize the value of each coordinate.
        feature.y = ((float)feature.y / (float)out1.w - _shift_y)*_rescale_y;
    }


    return keypoints;
}