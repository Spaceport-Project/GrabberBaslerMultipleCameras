
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <functional> 
#include <cmath>  
#include <memory>
#include <cstdint>
#include <exception>
#include <numeric>
// #include <Windows.h>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/bind/bind.hpp>
#include "BaslerMultipleCameras.h"
#include "SafeVector.h"
#include "magic_enum.hpp"


#ifdef DEBUG
#ifdef _MSC_VER 
#define DEBUG_PRINT(...) printf_s(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINT(...) do {} while (0)
#endif



template<typename T>
std::vector<std::size_t> tag_sort(const std::vector<T>& v)
{
    std::vector<std::size_t> result(v.size());
    std::iota(std::begin(result), std::end(result), 0);
    std::sort(std::begin(result), std::end(result),
            [&v](const auto & lhs, const auto & rhs)
            {
                return v[lhs] < v[rhs];
            }
    );
    return result;
}

// FBS Calculator
thread_local unsigned count = 0;
thread_local double last = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
#define FPS_CALC(_WHAT_, ncurrCameraIndex) \
do \
{ \
    double now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); \
    ++count; \
    if (now - last >= 1.0) \
    { \
      std::cerr << "\033[1;31m";\
      std::cerr << ncurrCameraIndex<< ". Camera,"<<" Average framerate("<< _WHAT_ << "): " << double(count)/double(now - last) << " fbs." <<  "\n"; \
      std::cerr << "\033[0m";\
      count = 0; \
      last = now; \
    } \
} while(false)

thread_local unsigned count_buf = 0;
thread_local unsigned counter = 0;

thread_local double last_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();

#define FPS_CALC_THREAD_BUF(_WHAT_, buff, ncurrCameraIndex) \
do \
{ \
    double now_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); \
    ++count_buf; \
    ++counter; \
    if (now_buf - last_buf >= 2.0) \
    { \
      std::cerr <<  ncurrCameraIndex<< ". Camera,"<<" Average framerate("<< _WHAT_ << "): " << double(count_buf)/double(now_buf - last_buf) << " Hz. Queue size: " << buff[ncurrCameraIndex].size () << " Frame Number: "<<counter <<"\n"; \
      count_buf = 0; \
      last_buf = now_buf; \
    } \
}while(false)


// #define FPS_CALC_BUF(_WHAT_, buff) \
// do \
// { \
//     static unsigned count_buf = 0;\
//     static unsigned counter = 0; \
//     static double last_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();\
//     double now_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); \
//     ++count_buf; \
//     ++counter; \
//     if (now_buf - last_buf >= 5.0) \
//     { \
//       std::cerr << "Average framerate("<< _WHAT_ << "): " << double(count_buf)/double(now_buf - last_buf) << " Hz. Queue size: " << buff.size () << " Frame Number: "<<counter <<"\n"; \
//       count_buf = 0; \
//       last_buf = now_buf; \
//     } \
// }while(false)


// BaslerMultipleCameras dialog
BaslerMultipleCameras::BaslerMultipleCameras( const std::string& cameraSettingsFile):
     
      m_tlFactory (CTlFactory::GetInstance())
    , m_sCameraSettingsFile(cameraSettingsFile)

    
   
{

    EnumDevices();

    if (m_nDeviceNum > 0)
    {
      
       m_bsCameras.Initialize(m_nDeviceNum);
       m_queueGrabRes.resize(m_nDeviceNum);
    
    } 
    else 
    {
        fprintf(stderr, "Exiting...\n");
        exit(0);

    }
    
}




// Thread Function for save images on disk for every camera
int BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun(int nCurCameraIndex)
{
    
        
         while(true) {
               std::this_thread::sleep_for(std::chrono::milliseconds(100));

                std::cout<<nCurCameraIndex<<" .cam, in consumer, queue size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;

                const auto buff_item = m_queueGrabRes[nCurCameraIndex].front();
                std::cout<<buff_item[120]<<std::endl;
                // m_queueGrabRes[nCurCameraIndex].pop_front();

                // std::cout<<nCurCameraIndex<<" .cam, FrameNum:"  << " "<<buff_item.second.get()[234]<<std::endl;
                // bool res = converter->convertAndEncodeBayerToH264(buff_item.second.get(), nCurCameraIndex, buff_item.first.nHostTimeStamp,  buff_item.first.nFrameNum);
                // if (res  )
                //         // converter->push
                //     converter->writeSingleFrame2MP4(nCurCameraIndex);
                FPS_CALC_THREAD_BUF ("Consuming from Buffer callback", m_queueGrabRes, nCurCameraIndex);


                if (m_bExit.load()) break; 
         }

         while (m_queueGrabRes[nCurCameraIndex].size() != 0){

                const auto buff_item = m_queueGrabRes[nCurCameraIndex].front();
                // printf("%d. Cam, Left Buffer Size: %d", nCurCameraIndex, m_queueGrabRes[nCurCameraIndex].size());
                m_queueGrabRes[nCurCameraIndex].pop_front();
                // bool res = converter->convertAndEncodeBayerToH264(buff_item.second.get(), nCurCameraIndex, buff_item.first.nHostTimeStamp,  buff_item.first.nFrameNum);
                // if (res )
                //     converter->writeSingleFrame2MP4(nCurCameraIndex);
                FPS_CALC_THREAD_BUF ("Consuming from Buffer callback", m_queueGrabRes, nCurCameraIndex);
                std::cout<<nCurCameraIndex<< ". Cam, Left Buffer Size:" << m_queueGrabRes[nCurCameraIndex].size()<<std::endl;


         }
        
        
        
       
   

        
    
    return 0;
}

//Thread function with GetImageBuffer API
int BaslerMultipleCameras::ThreadGrabFun(int nCurCameraIndex)
{
    
    std::this_thread::sleep_until(m_tWakeupTime);

    m_bsCameras[nCurCameraIndex].StartGrabbing();
    int c_countOfImagesToGrab = 100;
    for (uint32_t i = 0; i < c_countOfImagesToGrab && m_bsCameras[nCurCameraIndex].IsGrabbing(); i++) 
    {
        if (m_bExit.load()) 
            break;
        // This smart pointer will receive the grab result data.
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;
        m_bsCameras[nCurCameraIndex].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
        // intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
        if (ptrGrabResult->GrabSucceeded())
        {
            // std::cout<<"Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;
            // std::cout << "Camera " << nCurCameraIndex << ": " << m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetModelName() <<
            //     " (" << m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetIpAddress() << ")" << std::endl;

            // std::cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << std::endl;
            uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            size_t bufferSize = ptrGrabResult->GetBufferSize();
            std::shared_ptr<uint8_t[]>  tmpSharedptr(pImageBuffer);
            // std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
            // memcpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
            // std::cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << std::endl << std::endl;
            std::string filename= "GrabbedImage_" + std::to_string (i) + ".tiff";
            String_t file_name(filename.c_str());
            // std::cout<<file_name<<std::endl;
            // std::cout<<nCurCameraIndex<<" .cam, queue size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;

            m_queueGrabRes[nCurCameraIndex].push_back(tmpSharedptr);
            std::cout<<nCurCameraIndex<<" .cam, After queue size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;

            // CImagePersistence::Save( ImageFileFormat_Tiff, file_name, ptrGrabResult );
        }
        else
        {
            
            std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
        }



   
    } 
    m_bExit.store(true);
    return 0;
}




// Thread function with GetOneFrameTimeOut API




void BaslerMultipleCameras::EnumDevices()
{
    try
    {
        // Get the transport layer factory.
        m_pTL = dynamic_cast<IGigETransportLayer*>(m_tlFactory.CreateTl( BaslerGigEDeviceClass ));
    
        if (m_pTL->EnumerateDevices( m_allDeviceInfos ) == 0)
        {
            throw RUNTIME_EXCEPTION( "No GigE cameras present!" );
        }
        // Get all attached cameras.
        // TlFactory.EnumerateDevices( devices );
    }
    catch (const GenericException& e)
    {
        PYLON_UNUSED( e );

        std::cerr<<e.GetDescription()<<std::endl;
    }
    m_nDeviceNum = m_allDeviceInfos.size() ;
    for (unsigned int i = 0; i < m_nDeviceNum; i++) {
        m_mapSerials.insert(std::make_pair(i , m_allDeviceInfos[i].GetSerialNumber().c_str()));
 
    } 
    

}




//  Initialzation, include opening device
int BaslerMultipleCameras::OpenDevices()
{
    
    try
    {
        for (size_t i = 0; i < m_nDeviceNum; ++i)
        {
            m_bsCameras[i].Attach( m_tlFactory.CreateDevice( m_allDeviceInfos[i] ) );
            m_bsCameras[i].SetCameraContext( i );
            m_bsCameras[i].Open();
         

        }
    }  
    catch (const GenericException& e)
    {
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }
    return m_nExitCode;

}


int BaslerMultipleCameras::OpenDevicesInThreads()
{

    
    for (unsigned int  i = 0; i < m_nDeviceNum; i++)
    {
        m_tOpenDevicesThreads.push_back(std::make_unique<std::thread>(std::bind(&BaslerMultipleCameras::ThreadOpenDevicesFun, this, i)));
       
    }
    for (const auto &th :m_tOpenDevicesThreads)
    {
        th->join();
    }
    return m_nExitCode;


}

int BaslerMultipleCameras::ThreadOpenDevicesFun(int nCurCameraIndex) 
{
    try
    {
       
            m_bsCameras[nCurCameraIndex].Attach( m_tlFactory.CreateDevice( m_allDeviceInfos[nCurCameraIndex] ) );
            m_bsCameras[nCurCameraIndex].SetCameraContext(nCurCameraIndex );
            m_bsCameras[nCurCameraIndex].Open();
            
            // m_mapSerials.insert(std::make_pair(nCurCameraIndex , m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetSerialNumber().c_str()));



       
    }  
    catch (const GenericException& e)
    {
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }
    return m_nExitCode;
   
  


}

void BaslerMultipleCameras::CloseDevicesInThreads()
{

   

}

int BaslerMultipleCameras::ThreadCloseDevicesFun(int nCurCameraIndex)
{

   
   
   

    return 0; 

}



int BaslerMultipleCameras::ConfigureCameraSettings()
{
    boost::property_tree::ptree pt;

    // Read the JSON file
    std::ifstream file(m_sCameraSettingsFile);
    if (!file.good()) 
    {
        printf("Error in opening 'CameraSettings.json' file! Exiting... \n");
        m_nExitCode = 1;
        return m_nExitCode;
    }

    boost::property_tree::read_json(file, pt);
    int height = pt.get<int>("Height");
    int width = pt.get<int>("Width");
    float exposureTime = pt.get<float>("ExposureTime");
    float acquisitionFrameRate = pt.get<float>("AcquisitionFrameRate");
    float gain = pt.get<float>("Gain");
    std::string pixelFormat = pt.get<std::string>("PixelFormat");
    
    auto pixelFormatEnum = magic_enum::enum_cast<Basler_UniversalCameraParams::PixelFormatEnums>(pixelFormat);
  

    
    file.close();

    try
    {
        for (size_t i = 0; i < m_nDeviceNum; ++i)
        {
            
            if (pixelFormatEnum.has_value())
                m_bsCameras[i].PixelFormat.SetValue(pixelFormatEnum.value());
            else 
                std::cerr << "Pixel Format not set! Continueing with the default or previous value." << std::endl;

            m_bsCameras[i].PtpEnable.SetValue(false);
            
            m_bsCameras[i].BslPtpPriority1.SetValue(128);
            // Enable end-to-end delay measurement
            m_bsCameras[i].BslPtpProfile.SetValue(BslPtpProfile_DelayRequestResponseDefaultProfile);
            // Set the network mode to unicast
            m_bsCameras[i].BslPtpNetworkMode.SetValue(BslPtpNetworkMode_Multicast);
            m_bsCameras[i].BslPtpManagementEnable.SetValue(true);
            // Disable two-step operation
            m_bsCameras[i].BslPtpTwoStep.SetValue(false);
            m_bsCameras[i].PtpEnable.SetValue(true);
            while (m_bsCameras[i].GevSCPSPacketSize.GetValue()!=8000) {
				m_bsCameras[i].GevSCPSPacketSize.SetValue(8000);
			}

            m_bsCameras[i].Width.SetValue(width);
			m_bsCameras[i].Height.SetValue(height);
            m_bsCameras[i].ExposureTime.SetValue(exposureTime);
            m_bsCameras[i].GainSelector.SetValue(GainSelector_All);
            m_bsCameras[i].Gain.SetValue(gain);

            if (m_bsCameras[i].BslPeriodicSignalSource.GetValue() != BslPeriodicSignalSource_PtpClock ){
               printf("Clock source of periodic signal is not `PtpClock`\n");
               return 0;
            }
            // cout<<"ptp clock:"<<m_bsCameras[i].BslPeriodicSignalSource.GetValue()<<endl;

            m_bsCameras[i].BslPeriodicSignalPeriod.SetValue(1/acquisitionFrameRate  * 1e6);
            m_bsCameras[i].BslPeriodicSignalDelay.SetValue(0);
            m_bsCameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
            m_bsCameras[i].TriggerMode.SetValue(TriggerMode_On);
            m_bsCameras[i].TriggerSource.SetValue(TriggerSource_PeriodicSignal1);

        }
    }  
    catch (const GenericException& e)
    {
        // Error handling
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }

    
    return m_nExitCode;
    
   
}



// Close handle
int BaslerMultipleCameras::CloseDevices()
{

    try {
        m_bsCameras.Close(); 
        std::cout<<"Cameras Closed!"<<std::endl;
    }
    catch (const GenericException& e)
    {
        // Error handling
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }

    return m_nExitCode;
}


int BaslerMultipleCameras::Save2BufferThenDisk()
{
    
   

    for (unsigned int i = 0; i < m_nDeviceNum; i++)
    {
      
            // m_bStartConsuming = true;
            m_tConsumeThreads.push_back(std::make_unique<std::thread>(std::bind(&BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun, this, i)));
            if (m_tConsumeThreads[i] == nullptr)
            {
                printf("Create consume thread fail! DevIndex[%d]\r\n", i);
                return 1;
            }

        
    }

 
    return m_nExitCode;

}   





// Start grabbing
int BaslerMultipleCameras::StartGrabbing()
{
    auto now = std::chrono::system_clock::now();
    unsigned int duraSec = 1;
    std::chrono::seconds duration(duraSec);

    // Calculate the time at which you want to wake up
    m_tWakeupTime = now + duration;
    
    for (unsigned int i = 0; i < m_nDeviceNum; i++)
    {
        
        // m_bsCameras[i].StartGrabbing();

        m_tGrabThreads.push_back(std::make_unique<std::thread>(std::bind(&BaslerMultipleCameras::ThreadGrabFun, this, i)));
        if (i == 0 )
            printf("Grabbing will start %d seconds later!\n", duraSec);
        if (m_tGrabThreads[i] == nullptr)
        {
            printf("Create grab thread fail! DevIndex[%d]. Exiting...\r\n", i);
            return -1;
        }
        
    }
    
    
    
    // int c_countOfImagesToGrab = 10;
    // for (uint32_t i = 0; i < c_countOfImagesToGrab && m_bsCameras.IsGrabbing(); i++) 
    // {
    //     // This smart pointer will receive the grab result data.
    //     CBaslerUniversalGrabResultPtr ptrGrabResult;
    //      const int DefaultTimeout_ms = 5000;
    //     for (size_t j = 0; j < m_nDeviceNum && m_bsCameras.IsGrabbing(); ++j)
    //     {
    //             // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
    //             m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );

    //             // When the cameras in the array are created the camera context value
    //             // is set to the index of the camera in the array.
    //             // The camera context is a user-settable value.
    //             intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
    //             if (ptrGrabResult->GrabSucceeded())
    //             {
    //                 std::cout<<"Timestamp:"<<ptrGrabResult->GetTimeStamp()<<std::endl;
    //                     // Print the index and the model name of the camera.
    //                 std::cout << "Camera " << cameraIndex << ": " << m_bsCameras[cameraIndex].GetDeviceInfo().GetModelName() <<
    //                     " (" << m_bsCameras[cameraIndex].GetDeviceInfo().GetIpAddress() << ")" << std::endl;

    //                 // You could process the image here by accessing the image buffer.
    //                 std::cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << std::endl;
    //                 const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
    //                 std::cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << std::endl << std::endl;
    //                 std::string filename= "GrabbedImage_" + std::to_string (i) + ".tiff";
    //                 String_t file_name(filename.c_str());
    //                 std::cout<<file_name<<std::endl;
    //                 CImagePersistence::Save( ImageFileFormat_Tiff, file_name, ptrGrabResult );
    //             }
    //             else
    //             {
    //                 // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
    //                 // multiple images simultaneously. See note above c_maxCamerasToUse.
    //                 std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
    //             }
    
    //     }
    
    // }

   
    
   
   return m_nExitCode;

}

// Thread function for triggering




// Stop grabbing
int BaslerMultipleCameras::StopGrabbing()
{
  
   
  
    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (unsigned int  i = 0; i < m_nDeviceNum; i++)
    {
       
            // m_tSaveAsMP4Threads[i]->join();

            m_tGrabThreads[i]->join();
            m_tConsumeThreads[i]->join();
      
        
        
    }

    try {
        m_bsCameras.StopGrabbing(); 
        std::cout<<"Stop grabbing success!"<<std::endl;
    }
    catch (const GenericException& e)
    {
        // Error handling
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }
  
   
    return m_nExitCode;
   

}


// Software trigger

// GigE Action Command Trigger



