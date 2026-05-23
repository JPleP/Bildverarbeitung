#include "CameraManager.h"

//Initializes the Camera Manager Class. Usually called by the loop
void CameraManager::Init(){
    //Only create one instance of libcamera CameraManager per application
    CameraManager::_cameraManager = std::make_unique<libcamera::CameraManager>();
    CameraManager::_cameraManager->start();


    CameraManager::GetSetCamera();


    
}


//Responsible for getting a camera and setting the required configurations
void CameraManager::GetSetCamera(){
    //Internal vector to hold available cameras
    std::vector<std::shared_ptr<Camera>> cameras;

    //Iterate so long till a camera appears
    while(cameras = CameraManager::_cameraManager->cameras(); cameras.empty()){
        //No camera was found
        std::err << "No Camera. Trying in 100 ms."<< std::endl;
        // Sleep the current thread for 100 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    //Save the camera
    CameraManager::_camera = CameraManager::_cameraManager->get(cameras[0]->id());

    //Lock the camera for own use, so that no other app can use it.
    CameraManager::_camera->adquire();

    //Set the defautl template configuration of the camera streams. We get two: one for the detection and one for display. Way cheaper than conversion due to ISP.
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder,StreamRole::Viewfinder } );

    {
        //The first configuration is for the NN
        libcamera::StreamConfiguration &streamConfig = config->at(0);
        streamConfig.size.width = CameraManager::_NN._size.x;
        streamConfig.size.height = CameraManager::_NN._size.y;
        //Validate if the values are allowed
        if(auto result = config->validate(); result == libcamera::CameraConfiguration::Adjusted){
            //Change our internal reference
            _NN._size.x = config.size.width;
            _NN._size.y = config.size.height;
        } else if(result == libcamera::CameraConfiguration::Invalid){
            assert(false && "Invalid Camera Configuration for NN");
        }
    }
    {
        //The second configuration is for the display
        libcamera::StreamConfiguration &streamConfig = config->at(1);
        streamConfig.size.width = CameraManager::_Display._size.x;
        streamConfig.size.height = CameraManager::_Display._size.y;
        //Validate if the values are allowed
        if(auto result = config->validate(); result == libcamera::CameraConfiguration::Adjusted){
            //Change our internal reference
            _Display._size.x = config.size.width;
            _Display._size.y = config.size.height;
        } else if(result == libcamera::CameraConfiguration::Invalid){
            assert(false && "Invalid Camera Configuration for Display");
        }
    }   

    //Configure the Camera
    CameraManager::_camera->configure(config.get());

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
        _NN.stream = streamConfig.stream();
        int ret = allocator->allocate(_NN.stream);
        assert(ret >= 0 && "Can't allocate framebuffers for camera");
    }
    {
        libcamera::StreamConfiguration &streamConfig = config->at(1);
        _Display.stream = streamConfig.stream();
        int ret = allocator->allocate(_NN.stream);
        assert(ret >= 0 && "Can't allocate framebuffers for camera");
    }

    
    libcamera::Stream *stream = streamConfig.stream();
    const std::vector<std::unique_ptr<libcamera::FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<libcamera::Request>> requests;


    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<libcamera::Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        const std::unique_ptr<libcamera::FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request"
                << std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }


    CameraManager::_camera->requestCompleted.connect(requestComplete);

    camera->start();
    for (std::unique_ptr<libcamera::Request> &request : requests)
        camera->queueRequest(request.get());
}


void CameraManager::RequestNNComplete(libcamera::Request *request)
{
    //Chech that request finished succesfully
    if (request->status() == libcamera::Request::RequestCancelled)
        return;

    const std::map<const libcamera::Stream *, libcamera::FrameBuffer *> &buffers = request->buffers();

    request->reuse(libcamera::Request::ReuseBuffers);
    CameraManager::_camera->queueRequest(request);

}


void CameraManager::End(){
    CameraManager::_camera->stop();
    allocator->free(stream);
    delete allocator;
    CameraManager::_camera->release();
    CameraManager::_camera.reset();
    CameraManager::_cameraManager->stop();
}