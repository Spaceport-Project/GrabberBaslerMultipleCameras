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
// #include "SharedQueue.h"
#include "ImageBuffer.h"
// #include "SafeVector.h"
#include "SafeQueue.h"
using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;

// Example of an image event handler.


struct DATA{
    std::vector<uint8_t>  image;
    u_int64_t timeStamp=0;
    size_t imageSize=0;
};
class BaslerMultipleCameras 
{
// Construction
public:
    
    using thread = std::unique_ptr<std::thread>;
    using threadVector = std::vector<std::unique_ptr<std::thread>>;
    using timePoint = std::chrono::system_clock::time_point;
    using byteArrayVector = std::vector<std::shared_ptr<uint8_t[]>> ;
    using condVector = std::vector<std::condition_variable>;
    static bool m_bExit;

	BaslerMultipleCameras(const std::string&);	      
   
private:
    unsigned int            m_uDeviceNum = 0;
    const std::string&      m_sCameraSettingsFile;
   
   
    threadVector            m_tGrabThreads;
    thread                  m_tGrabThread;
    threadVector            m_tConsumeThreads;
   
    thread                  m_tCheckBuffThread;
    threadVector            m_tOpenDevicesThreads;
    threadVector            m_tWriteMP4Threads;


    std::mutex              m_mWriteMp4Mutex;
   
  
    std::map<int, std::string> m_mapSerials; 
    std::map<int, std::string> m_mapModels; 
    // std::vector<ImageBuffer<CBaslerUniversalGrabResultPtr> > m_queueGrabRes;
    std::vector<SafeQueue<DATA>>  m_queueGrabRes;
    // std::shared_ptr<Sa<DATA>[]> m_queueGrabRes;

    int m_nExitCode = 0;
    CBaslerUniversalInstantCameraArray m_bsCameras;
    IGigETransportLayer* m_pTL = nullptr;
    CTlFactory &m_tlFactory;
    // CTlFactory m_tlFactoryInst;
    DeviceInfoList_t m_allDeviceInfos;
    std::chrono::system_clock::time_point m_tWakeupTime;
    uint32_t DeviceKey;
    int m_uHeight ;
    int m_uWidth ;
    float m_fExposureTime ;
    float m_fAcquisitionFrameRate;
    float m_fGain;
    std::string m_sPixelFormat;
  
   
  
 
public:
    void EnumDevices();

    int OpenDevices();
    int StartGrabbing();
    int ThreadSingleGrabFun();
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
    // std::vector<ImageBuffer<std::unique_ptr<uint8_t[]> >> & GetQueueVectors() {return m_queueGrabRes;};
    friend class  CBaslerImageEventHandler;

private:
   
    void Write2H264FromBayer(int numWriteThreads);




};


class CBaslerImageEventHandler : public CImageEventHandler
{
public:
    using  SharedQueueRef = std::vector<SafeQueue<DATA>>  &;
    CBaslerImageEventHandler(SharedQueueRef  varSharedQueueVec):
    varSharedQueueVec_(varSharedQueueVec)
    
    {};

    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult )
    {
        u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();

        // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
        uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
        size_t bufferSize = ptrGrabResult->GetBufferSize();
        int cameraIndex = ptrGrabResult->GetCameraContext();
        size_t cameraPadding = ptrGrabResult->GetPaddingX();
        int width = ptrGrabResult->GetWidth();
        int height = ptrGrabResult->GetHeight();

        // std::cout<<"cameraPadding:"<<cameraPadding<< " "<<ptrGrabResult->GetWidth()<<std::endl;


        //std::shared_ptr<uint8_t[]>  tmpSharedptr(pImageBuffer);
        // std::unique_ptr<uint8_t[]>  tmpUniqueptr (pImageBuffer);
        // std::vector<uint8_t>  tmpSharedptr (bufferSize);
        tmpSharedptr.resize(bufferSize);
        memcpy(tmpSharedptr.data(), pImageBuffer, bufferSize);
        // uint8_t *imageBuffer= new u_int8_t[width*height*sizeof(uint8_t)];
        // memcpy (imageBuffer, pImageBuffer, bufferSize);
        DATA data{tmpSharedptr, timeStamp, bufferSize};
        varSharedQueueVec_[cameraIndex].enqueue( data);
        // ptrGrabResult.Release();
        
        // std::unique_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
        // memcpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
        // std::cout << std::endl;
        // std::cout << std::endl;
    }

   
    private :
        std::vector<SafeQueue<DATA>> & varSharedQueueVec_;
         std::vector<uint8_t>  tmpSharedptr ;
};