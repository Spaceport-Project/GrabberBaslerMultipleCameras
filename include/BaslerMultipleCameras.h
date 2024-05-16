// header file

#pragma once
#include <stdio.h>
#include <thread>
#include <condition_variable>
#include <memory>
#include <cstdint>
#include <map>
#include <queue>
#include <cstdlib>
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#   include <pylon/PylonGUI.h>
#endif

#include <pylon/BaslerUniversalInstantCameraArray.h>
// #include <pylon/gige/PylonGigEIncludes.h>
#include <pylon/Info.h>
#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/ActionTriggerConfiguration.h>
#include <pylon/gige/BaslerGigEDeviceInfo.h>
#include "Bayer2H264Converter.h"
#include "SharedQueue.h"
#include "ImageBuffer.h"
#include "SafeVector.h"
using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;



class BaslerMultipleCameras 
{
// Construction
public:
    
    using thread = std::unique_ptr<std::thread>;
    using threadVector = std::vector<std::unique_ptr<std::thread>>;
    using timePoint = std::chrono::system_clock::time_point;
    using byteArrayVector = std::vector<std::shared_ptr<uint8_t[]>> ;
    using condVector = std::vector<std::condition_variable>;
    static std::atomic<bool> m_bExit;

	BaslerMultipleCameras(const std::string&);	      
   
private:
    unsigned int            m_nDeviceNum = 0;
    const std::string&      m_sCameraSettingsFile;
   
   
    threadVector            m_tGrabThreads;
    threadVector            m_tConsumeThreads;
   
    thread                  m_tCheckBuffThread;
    threadVector            m_tOpenDevicesThreads;
    threadVector            m_tWriteMP4Threads;


    std::mutex              m_mWriteMp4Mutex;
   
  
    std::map<int, std::string> m_mapSerials; 
    std::map<int, std::string> m_mapModels; 
    // std::vector<SharedQueue<CBaslerUniversalGrabResultPtr> > m_queueGrabRes;
    std::vector<SharedQueue<std::shared_ptr<uint8_t[]> >>  m_queueGrabRes;
    int m_nExitCode = 0;
    CBaslerUniversalInstantCameraArray m_bsCameras;
    IGigETransportLayer* m_pTL = nullptr;
    CTlFactory &m_tlFactory;
    // CTlFactory m_tlFactoryInst;
    DeviceInfoList_t m_allDeviceInfos;
    std::chrono::system_clock::time_point m_tWakeupTime;

   
  
 
public:
    void EnumDevices();

    int OpenDevices();
    int StartGrabbing();
    int CloseDevices();
    int StopGrabbing();
    int ConfigureCameraSettings();
    
    int  Save2BufferThenDisk();
    int OpenDevicesInThreads();
    void CloseDevicesInThreads();
    
    

    int ThreadConsumeAnWrite2DiskAsMp4Fun(int );
    int ThreadGrabFun(int nCurCameraIndex);
    int ThreadOpenDevicesFun(int);
    int ThreadCloseDevicesFun(int );

private:
   
    void Write2H264FromBayer(int numWriteThreads);




};
