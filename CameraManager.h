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
};



class CameraManager{
    public:
    static void Init();
    static void End();

    static void GetSetCamera();

    static std::shared_ptr<libcamera::Camera> _camera;
    static std::unique_ptr<libcamera::CameraManager> _cameraManager;
    static std::unique_ptr<libcamera::FrameBufferAllocator> _allocator;
    static std::vector<std::unique_ptr<libcamera::Request>> _requests;

    static int64_t frameCount = 0;
     

    static VideoStream _NN;
    static VideoStream _Display;

    static void requestComplete(libcamera::Request *request);
};