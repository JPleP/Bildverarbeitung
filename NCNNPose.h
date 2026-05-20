#pragma once


#pragma once
#include "mat.h"
#include "net.h"
#include <cfloat>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>

#include "utils.h"




class NCNNPose{
public:
    NCNNPose(const char* paramFile, const char* binFile,int width, int height);
    //Input a open cv frame and forward the model interference
    std::vector<Keypoint> GetData(cv::Mat frame);

    //The wrapper around the network
    ncnn::Net _net;

    //The input size of the Model
    int _width, _height;
};