#pragma once
#include "mat.h"
#include "net.h"
#include <cfloat>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>

#include "utils.h"



class NCNN {
public:
    NCNN(const char* paramFile, const char* binFile, int width, int height);
    ncnn::Extractor PrepareInput(const cv::Mat& frame);

    //The wrapper around the network
    ncnn::Net _net;

    //The input size of the Model
    int _width, _height;
    float _rescale_x, _rescale_y;
    float _shift_x, _shift_y;
};




class NCNNDet : public NCNN{
public:
    using NCNN::NCNN;
    //Input a open cv frame and forward the model interference,Postprocess raw tensor from detection to BBox. For speed, only consider those above threshold
    std::vector<BBox> GetData(ncnn::Extractor ex,const float& threshold = 0.3);
    BBox Idx2BBox(const ncnn::Mat& matrix, int x, int y, float stride);
};

class NCNNPose : public NCNN{
public:
    using NCNN::NCNN;
    //Input a open cv frame and forward the model interference,Postprocess raw tensor from detection to BBox. For speed, only consider those above threshold
    std::vector<Keypoint> GetData(ncnn::Extractor ex);
};


class Body2DPoseEst : public NCNNPose{
    public:
    enum KEY_LABELS{
        nose = 0 ,
        left_eye,right_eye,
        left_ear,right_ear,
        left_shoulder,right_shoulder,
        left_elbow,right_elbow,
        left_wrist, right_wrist,
        left_hip, right_hip,
        left_knee, right_knee,
        left_ankle, right_ankle
    };

};