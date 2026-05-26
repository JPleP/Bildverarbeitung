#include "CameraManager.h"

std::shared_ptr<libcamera::Camera> CameraManager::_camera;
std::unique_ptr<libcamera::CameraManager> CameraManager::_cameraManager;
std::unique_ptr<libcamera::FrameBufferAllocator> CameraManager::_allocator;
std::vector<std::unique_ptr<libcamera::Request>> CameraManager::_requests;

VideoStream CameraManager::_NN ;
VideoStream CameraManager::_Display;

static const char* tag = "CameraManager";

//Initializes the Camera Manager Class. Usually called by the loop
void CameraManager::Init(){
    LOGI(tag,"Starting");
    CameraManager::_Display = {Vec2i{480,640},nullptr};
    CameraManager::_NN = {Vec2i{320,320},nullptr};


    //Only create one instance of libcamera CameraManager per application
    CameraManager::_cameraManager = std::make_unique<libcamera::CameraManager>();
    CameraManager::_cameraManager->start();

    CameraManager::GetSetCamera();


    LOGI(tag,"Finished setting up");
}


//Responsible for getting a camera and setting the required configurations
void CameraManager::GetSetCamera(){
    LOGI(tag,"Setting Up Camera");
    //Internal vector to hold available cameras
    std::vector<std::shared_ptr<libcamera::Camera>> cameras = CameraManager::_cameraManager->cameras();;

    //Iterate so long till a camera appears
    while(cameras.empty()){
        //No camera was found
        std::cerr << "No Camera. Trying in 100 ms."<< std::endl;
        // Sleep the current thread for 100 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOGI(tag,"Setting Camera");
    //Save the camera
    CameraManager::_camera = CameraManager::_cameraManager->get(cameras[0]->id());

    //Lock the camera for own use, so that no other app can use it.
    CameraManager::_camera->acquire();
    LOGI(tag,"Camera Acquired");

    //Set the defautl template configuration of the camera streams. We get two: one for the detection and one for display. Way cheaper than conversion due to ISP.
    std::unique_ptr<libcamera::CameraConfiguration> config = CameraManager::_camera->generateConfiguration( { libcamera::StreamRole::Viewfinder,libcamera::StreamRole::Viewfinder } );

    {
        //The first configuration is for the NN
        libcamera::StreamConfiguration &streamConfig = config->at(0);
        streamConfig.size.width = CameraManager::_NN._size.x;
        streamConfig.size.height = CameraManager::_NN._size.y;
        streamConfig.pixelFormat = libcamera::formats::RGB888;
        streamConfig.bufferCount = 4;
        streamConfig.bu
        //Validate if the values are allowed
        if(auto result = config->validate(); result == libcamera::CameraConfiguration::Adjusted){
            //Change our internal reference
            _NN._size.x = streamConfig.size.width;
            _NN._size.y = streamConfig.size.height;
        } else if(result == libcamera::CameraConfiguration::Invalid){
            assert(false && "Invalid Camera Configuration for NN");
        }
    }
    {
        //The second configuration is for the display
        libcamera::StreamConfiguration &streamConfig = config->at(1);
        streamConfig.size.width = CameraManager::_Display._size.x;
        streamConfig.size.height = CameraManager::_Display._size.y;
        streamConfig.pixelFormat = libcamera::formats::RGB888;
        streamConfig.bufferCount = 4;
        //Validate if the values are allowed
        if(auto result = config->validate(); result == libcamera::CameraConfiguration::Adjusted){
            //Change our internal reference
            _Display._size.x = streamConfig.size.width;
            _Display._size.y = streamConfig.size.height;
        } else if(result == libcamera::CameraConfiguration::Invalid){
            assert(false && "Invalid Camera Configuration for Display");
        }
    }   
    LOGI(tag,"Configuration Validated");
    //Configure the Camera
    CameraManager::_camera->configure(config.get());

    LOGI(tag,"Configuration Set");
    //If we still own an allocator, delete it
    if(CameraManager::_allocator){
        if(_NN._stream)
            CameraManager::_allocator->free(_NN._stream);
        if(_Display._stream)
            CameraManager::_allocator->free(_Display._stream);
        CameraManager::_allocator.reset();
    }
    CameraManager::_allocator = std::make_unique<libcamera::FrameBufferAllocator>(CameraManager::_camera);

    {
        libcamera::StreamConfiguration &streamConfig = config->at(0);
        _NN._stream = streamConfig.stream();
        int ret = CameraManager::_allocator->allocate(_NN._stream);
        assert(ret >= 0 && "Can't allocate framebuffers for camera");
    }
    {
        libcamera::StreamConfiguration &streamConfig = config->at(1);
        _Display._stream = streamConfig.stream();
        int ret = CameraManager::_allocator->allocate(_Display._stream);
        assert(ret >= 0 && "Can't allocate framebuffers for camera");
    }

    LOGI(tag,"Buffer Allocated");

    
    const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &nnBuffers = _allocator->buffers(_NN._stream);
    const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &displayBuffers = _allocator->buffers(_Display._stream);

     
    for (unsigned int i = 0; i < std::min(nnBuffers.size(),displayBuffers.size()); ++i) {
        std::unique_ptr<libcamera::Request> request = CameraManager::_camera->createRequest();
        if (!request)
            assert(false && "Can't create request");

        int ret = request->addBuffer(_NN._stream, nnBuffers[i].get());
        if(ret < 0)
            assert(false && "Can't set NN buffer for request");
        ret = request->addBuffer(_Display._stream, displayBuffers[i].get());
        if(ret < 0)
            assert(false && "Can't set Display buffer for request");

        CameraManager::_requests.push_back(std::move(request));
    }
    LOGI(tag,"Requests Allocated");

    CameraManager::_camera->requestCompleted.connect(requestComplete);



    std::unique_ptr<libcamera::ControlList> camcontrols = std::unique_ptr<libcamera::ControlList>(new libcamera::ControlList());
    camcontrols->set(controls::FrameDurationLimits, libcamera::Span<const std::int64_t, 2>({33333, 33333}));
    CameraManager::_camera->start(camcontrols.get());
    for (std::unique_ptr<libcamera::Request> &request : CameraManager::_requests)
        CameraManager::_camera->queueRequest(request.get());
    
    LOGI(tag,"Camera started and requests queued");

}


void CameraManager::requestComplete(libcamera::Request *request)
{
    //Chech that request finished succesfully
    if (request->status() == libcamera::Request::RequestCancelled)
        return;
    //Get the buffers
    const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> &buffers = request->buffers();
    //Get the display buffer


    frameCount++;
    request->reuse(libcamera::Request::ReuseBuffers);
    CameraManager::_camera->queueRequest(request);
    


}


void CameraManager::End(){
    LOGI(tag,"Stopping");
    CameraManager::_camera->stop(); LOGI(tag,"Camera Stopped");
     //If we still own an allocator, delete it
    if(CameraManager::_allocator){
        if(_NN._stream)
            CameraManager::_allocator->free(_NN._stream);
        if(_Display._stream)
            CameraManager::_allocator->free(_Display._stream);
        CameraManager::_allocator.reset();
    }
    CameraManager::_camera->release();
    CameraManager::_camera.reset(); LOGI(tag,"Camera freed");
    CameraManager::_cameraManager->stop();
    CameraManager::_cameraManager.reset();
    LOGI(tag,"Stopped");
}