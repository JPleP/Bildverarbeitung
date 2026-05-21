#include "Camera.h"



Camera::Camera():
_videoStream(0){
    _resolution = cv::Size((int) _videoStream.get(cv::CAP_PROP_FRAME_WIDTH),
                    (int) _videoStream.get(cv::CAP_PROP_FRAME_HEIGHT));
}

void Camera::operator>>(cv::Mat& frame){
    _videoStream >> frame;
}