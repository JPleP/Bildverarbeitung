#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>
#include "utils.h"

struct VideoStream{
    Vec2i _size;
    libcamera::Stream* _stream;

}



class CameraManager{
    public:
    void Init();
    void End();

    void GetSetCamera();

    static void RunLoop(RPiCamApp& app);
    static void operator>>(cv::Mat& frame);

    static std::shared_ptr<libcamera::Camera> _camera;
    static std::unique_ptr<libcamera::CameraManager> _cameraManager;
    static std::unique_ptr<libcamera::FrameBufferAllocator> _allocator;


    static VideoStream _NN = {{320,320}};
    static VideoStream _Display = {{480,640}};

    void RequestNNComplete(libcamera::Request *request);
};