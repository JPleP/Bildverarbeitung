#include "CameraManager.h"



int main(){
    CameraManager::Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    CameraManager::End();


}