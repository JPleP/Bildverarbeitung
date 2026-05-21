#pragma once
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>


#include <opencv2/videoio.hpp> //For video input
#include <libcamera/libcamera.h>


class Camera{
    public:
    Camera();
    void operator>>(cv::Mat& frame);

    cv::VideoCapture _videoStream;
    cv::Size _resolution;
};