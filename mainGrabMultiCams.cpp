//#include <pthread.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <condition_variable>
#include <signal.h>
#include "BaslerMultipleCameras.h"
#include "CircularBuffer.h"



std::atomic<bool> BaslerMultipleCameras::m_bExit = false;


void ctrlC (int)
{
  printf ("\nCtrl-C detected, exit condition set to true.\n");
  BaslerMultipleCameras::m_bExit.store(true);
}

int main(int argc, char *argv[]) {
    int exit_code = 0;
    if (argc != 2) {
        printf("Usage: %s <path/to/CameraSettings.json>\n", argv[0]);

        return -1;

    } 
    std::string cameraSettingsFile(argv[1]);
    ;


    PylonInitialize();

    std::unique_ptr<BaslerMultipleCameras> baslerCams (new BaslerMultipleCameras(cameraSettingsFile));

    signal (SIGINT, ctrlC);
    
    // if (baslerCams->OpenDevices() != exit_code) 
    // {
    //   return -1;
    // }
    if (baslerCams->OpenDevicesInThreads() != exit_code) 
    {
      return -1;
    }

    if (baslerCams->ConfigureCameraSettings() != exit_code) 
    {
      return -1;
    }
   
 

    if (baslerCams->StartGrabbing() != exit_code) 
    {
      return -1;
    }

    // return 0;

    if (baslerCams->Save2BufferThenDisk() != exit_code) 
    {
      return -1;
    }

   
     
    if (baslerCams->StopGrabbing() == -1 )
    {
        return -1;
    } 
    
    baslerCams->CloseDevices();
   
    

   


    return 0;


}
