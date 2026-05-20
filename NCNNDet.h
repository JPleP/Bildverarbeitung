#pragma once
#include "mat.h"
#include "net.h"
#include <cfloat>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>

#include "utils.h"




class NCNNDet{
public:
    NCNNDet(const char* paramFile, const char* binFile, int width, int height);
    //Input a open cv frame and forward the model interference
    std::vector<ncnn::Mat> GetData(cv::Mat frame);
    //Postprocess raw tensor from detection to BBox. For speed, only consider those above threshold
    std::vector<BBox> GetBBoxes(const std::vector<ncnn::Mat> &tensors, const float& threshold = 0.3);

    //The wrapper around the network
    ncnn::Net _net;

    //The input size of the Model
    int _width, _height;
};