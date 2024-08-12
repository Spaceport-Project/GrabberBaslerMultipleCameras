//#include <pthread.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <condition_variable>
#include <signal.h>
#include "BaslerMultipleCameras.h"



// std::atomic<bool> BaslerMultipleCameras::m_bExit{false};
bool BaslerMultipleCameras::m_bExit=false;

// WaitObjectEx BaslerMultipleCameras::m_waitObject(WaitObjectEx::Create());
std::atomic<bool> BayerToH264ConverterNvidiaCodec::exit_flag{false};
void ctrlC (int)
{
  
  printf ("\nCtrl-C detected, exit condition set to true.\n");
  // BaslerMultipleCameras::m_bExit.store(true, std::memory_order_release);
  BayerToH264ConverterNvidiaCodec::exit_flag.store(true, std::memory_order_release);
  BaslerMultipleCameras::m_bExit = true;

  // BaslerMultipleCameras::m_waitObject.Signal();


}

bool isNinetyPercentRAMUsed() {
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        std::cerr << "Could not open /proc/meminfo" << std::endl;
        return false;
    }

    long totalMemory = 0;
    long availableMemory = 0;
    std::string key;
    long value;
    std::string unit;

    while (meminfo >> key >> value >> unit) {
        if (key == "MemTotal:") {
            totalMemory = value;
        } else if (key == "MemAvailable:") {
            availableMemory = value;
        }
    }

    meminfo.close();

    if (totalMemory == 0) {
        std::cerr << "Could not read total memory from /proc/meminfo" << std::endl;
        return false;
    }

    long usedMemory = totalMemory - availableMemory;
    double usedMemoryPercentage = (static_cast<double>(usedMemory) / totalMemory) * 100;

    return usedMemoryPercentage >= 92.0;
}

void checkMemoryUsage() {
    while (true) {
        if (isNinetyPercentRAMUsed()) {
            std::cout << "90% or more of the RAM is used." << std::endl;
            std::cout<<"Exiting from Grabbing Tool!"<<std::endl;
            BayerToH264ConverterNvidiaCodec::exit_flag.store(true, std::memory_order_release);
            BaslerMultipleCameras::m_bExit = true;
            return;

        } 
        // else {
        //      std::cout << "Less than 90% of the RAM is used." << std::endl;
        // }

        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char *argv[]) {
    int exit_code = 0;
    if (argc != 2) {
        printf("Usage: %s <path/to/CameraSettings.json>\n", argv[0]);

        return -1;

    } 
    // gst_init(&argc, &argv);

   

    std::string cameraSettingsFile(argv[1]);

    std::thread memoryChecker(checkMemoryUsage);

    memoryChecker.detach();


    PylonInitialize();
    

    // std::unique_ptr<BaslerMultipleCameras> baslerCams (new BaslerMultipleCameras(cameraSettingsFile));
    BaslerMultipleCameras * baslerCams  = new BaslerMultipleCameras(cameraSettingsFile);

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


    // if (baslerCams->Save2BufferThenDisk() != exit_code) 
    // {
    //   return -1;
    // }

   
     
    if (baslerCams->StopGrabbing() == -1 )
    {
        return -1;
    } 
    
    // baslerCams->CloseDevices();
    delete baslerCams;
    PylonTerminate();




    return 0;


}
