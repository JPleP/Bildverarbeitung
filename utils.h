#pragma once
#include <opencv2/core/types.hpp>


struct BBox{
    cv::Point2f _lt;
    cv::Point2f _br;
    float _prob = 0;
};


// Struct to hold final coordinate results
struct Keypoint {
    float x;
    float y;
};