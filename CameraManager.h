#pragma once

#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
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
private:
    //Get all available camera and sets everything up.
    static void GetSetCamera();

    static std::shared_ptr<libcamera::Camera> _camera;
    static std::unique_ptr<libcamera::CameraManager> _cameraManager;
    static std::unique_ptr<libcamera::FrameBufferAllocator> _allocator;
    static std::vector<std::unique_ptr<libcamera::Request>> _requests;

    //Internal frame counter
    static int64_t _frameCount = 0;
     
    //Specific data for each stream.
    static VideoStream _NN;
    static VideoStream _Display;

public:
    //The data relevant for the display.
    static std::mutex _displayLock;
    static std::unique_ptr<RGB[]> _displayImage;
    static Recti _userPosition;


public:
    //Function which receives the completed requests from the libcamera.
    static void RequestComplete(libcamera::Request *request);
};