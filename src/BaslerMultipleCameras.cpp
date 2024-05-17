
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

    if (m_uDeviceNum > 0)
    {
        m_bsCameras.Initialize(m_uDeviceNum);
        // std::shared_ptr<ImageBuffer<DATA>[]> 
        // m_queueGrabRes.reset(new ImageBuffer<DATA>[m_uDeviceNum]);

        m_queueGrabRes.resize(m_uDeviceNum, SafeQueue<DATA>());
        std::cout<<"step 0"<<std::endl;
        // for (int i = 0; m_uDeviceNum; i++) {
        //     // m_queueGrabRes.push_back( ImageBuffer<DATA>());

        //     // m_queueGrabRes[i].setCapacity();
        //     //  m_queueGrabRes[i].setCapacity(1000);

        // } 
        std::cout<<"step 1"<<std::endl;

    
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
    
        unsigned int i =0;
        while(true) {
               
                if (m_bExit ) {
                    break; 
                }

                const DATA buff_item = m_queueGrabRes[nCurCameraIndex].dequeue();
                int imageSize = buff_item.imageSize;
                if (nCurCameraIndex == 3) {

                    // printf(" Buffer:%d", buff_item.image[0]);
                     std::cout<<nCurCameraIndex<<". Cam, Buffer Size:"<< std::hex<<buff_item.image[0]<<std::endl;
                }
                
                // if (m_queueGrabRes[nCurCameraIndex].size() % 20 == 0)
                // {
                //    std::cout<<nCurCameraIndex<<". Cam, Buffer Size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;
                // }
               
                // std::this_thread::sleep_for(std::chrono::milliseconds(50));
                std::string filename= "GrabbedImage_" + std::to_string (i) + "_" + std::to_string (nCurCameraIndex) + ".tiff";
                String_t file_name(filename.c_str());
                std::cout<< nCurCameraIndex<<".Cam, timeStamp:"<<buff_item.timeStamp<<std::endl;
                CImagePersistence::Save(ImageFileFormat_Tiff, file_name, (void *)buff_item.image.data(), imageSize,  PixelType_BayerRG8, 4096, 3000, 0, ImageOrientation_TopDown );
              
                // bool res = converter->convertAndEncodeBayerToH264(buff_item.second.get(), nCurCameraIndex, buff_item.first.nHostTimeStamp,  buff_item.first.nFrameNum);
                // if (res  )
                //         // converter->push
                //     converter->writeSingleFrame2MP4(nCurCameraIndex);
                // FPS_CALC_THREAD_BUF ("Consuming from Buffer callback", m_queueGrabRes, nCurCameraIndex);

                i++;
               
         }
         while (m_queueGrabRes[nCurCameraIndex].size() != 0){
            const DATA &buff_item = m_queueGrabRes[nCurCameraIndex].dequeue();
            int imageSize = buff_item.imageSize;
            std::string filename= "GrabbedImage_" + std::to_string (i) + "_" + std::to_string (nCurCameraIndex) + ".tiff";
            String_t file_name(filename.c_str());
            std::cout<< nCurCameraIndex<<".Cam, timeStamp:"<<buff_item.timeStamp<<std::endl;
            CImagePersistence::Save(ImageFileFormat_Tiff, file_name, (void *)buff_item.image.data(), imageSize,  PixelType_BayerRG8, 4096, 3000, 0, ImageOrientation_TopDown );
              
            i++;
         }
        
        
        
       
   

        
    
    return 0;
}

//Thread function with GetImageBuffer API
int BaslerMultipleCameras::ThreadGrabFun(int nCurCameraIndex)
{
    
    // std::this_thread::sleep_until(m_tWakeupTime);

    // m_bsCameras[nCurCameraIndex].StartGrabbing();
    int c_countOfImagesToGrab = 1000000;
    for (uint32_t i = 0; i < c_countOfImagesToGrab && m_bsCameras[nCurCameraIndex].IsGrabbing(); i++) 
    {
        if (m_bExit) 
            break;
        // This smart pointer will receive the grab result data.
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;
        m_bsCameras[nCurCameraIndex].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
        // intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
        if (ptrGrabResult->GrabSucceeded())
        {
            if (i %10 == 0 /*&& (nCurCameraIndex ==2 || nCurCameraIndex ==1) */) 
                std::cout<<nCurCameraIndex<<". Cam, Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;
            // std::cout << "Camera " << nCurCameraIndex << ": " << m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetModelName() <<
            //     " (" << m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetIpAddress() << ")" << std::endl;

            // std::cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << std::endl;
            uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            size_t bufferSize = ptrGrabResult->GetBufferSize();
            // std::shared_ptr<uint8_t[]>  tmpSharedptr(pImageBuffer);
            std::unique_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
            memcpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
            // std::cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << std::endl << std::endl;
            std::string filename= "GrabbedImage_" + std::to_string (i) + ".tiff";
            String_t file_name(filename.c_str());
            // std::cout<<file_name<<std::endl;
            // std::cout<<nCurCameraIndex<<" .cam, queue size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;

            // m_queueGrabRes[nCurCameraIndex].push_back(tmpSharedptr);
            // std::cout<<nCurCameraIndex<<" .cam, After queue size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;

            // CImagePersistence::Save( ImageFileFormat_Tiff, file_name, ptrGrabResult );
        }
        else
        {
            
            std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
        }
        ptrGrabResult.Release();



   
    } 
    m_bExit=true;
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
        // m_tlFactory.EnumerateDevices( m_allDeviceInfos);
    }
    catch (const GenericException& e)
    {
        PYLON_UNUSED( e );

        std::cerr<<e.GetDescription()<<std::endl;
    }
    m_uDeviceNum = m_allDeviceInfos.size() ;
    std::cout<<m_uDeviceNum<<" GigE Cameras Found!"<<std::endl;
    for (unsigned int i = 0; i < m_uDeviceNum; i++) {
        std::cout<<i+1<<".Cam Serial Num:"<<m_allDeviceInfos[i].GetSerialNumber().c_str()<<std::endl;
        m_mapSerials.insert(std::make_pair(i , m_allDeviceInfos[i].GetSerialNumber().c_str()));
 
    } 
    

}




//  Initialzation, include opening device
int BaslerMultipleCameras::OpenDevices()
{
    
    try
    {
        for (size_t i = 0; i < m_uDeviceNum; ++i)
        {
            m_bsCameras[i].Attach( m_tlFactory.CreateDevice( m_allDeviceInfos[i] ) );
            // m_bsCameras[i].RegisterImageEventHandler( new CBaslerImageEventHandler(m_queueGrabRes), RegistrationMode_Append, Cleanup_Delete );
            m_bsCameras[i].GrabCameraEvents = true;
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
    // srand( (unsigned) time( NULL ) );
    // DeviceKey = rand();
    
    for (unsigned int  i = 0; i < m_uDeviceNum; i++)
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
           

        // For this sample we configure all cameras to be in the same group.
            // const uint32_t GroupKey = 0x112233;
            // const uint32_t AllGroupMask( 0xffffffff ); 
            m_bsCameras[nCurCameraIndex].Attach( m_tlFactory.CreateDevice( m_allDeviceInfos[nCurCameraIndex] ) );
            // m_bsCameras[nCurCameraIndex].RegisterConfiguration( new CActionTriggerConfiguration( DeviceKey, GroupKey, AllGroupMask ), RegistrationMode_Append, Cleanup_Delete );

            m_bsCameras[nCurCameraIndex].SetCameraContext(nCurCameraIndex );
            m_bsCameras[nCurCameraIndex].RegisterImageEventHandler( new CBaslerImageEventHandler(m_queueGrabRes), RegistrationMode_Append, Cleanup_Delete );
            m_bsCameras[nCurCameraIndex].GrabCameraEvents = true;

            m_bsCameras[nCurCameraIndex].Open();
              // Check if the device supports events.
            if (!m_bsCameras[nCurCameraIndex].EventSelector.IsWritable())
            {
                throw RUNTIME_EXCEPTION( "The device doesn't support events." );
            }

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
    m_uHeight = pt.get<int>("Height");
    m_uWidth = pt.get<int>("Width");
    m_fExposureTime = pt.get<float>("ExposureTime");
    m_fAcquisitionFrameRate = pt.get<float>("AcquisitionFrameRate");
    m_fGain = pt.get<float>("Gain");
    m_sPixelFormat = pt.get<std::string>("PixelFormat");
    
    auto pixelFormatEnum = magic_enum::enum_cast<Basler_UniversalCameraParams::PixelFormatEnums>(m_sPixelFormat);
  

    
    file.close();

    try
    {
        for (size_t i = 0; i < m_uDeviceNum; ++i)
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
            while (m_bsCameras[i].GevSCPSPacketSize.GetValue()!=1500) {
				m_bsCameras[i].GevSCPSPacketSize.SetValue(1500);
			}
            m_bsCameras[i].GevSCPD.SetValue(512);
            m_bsCameras[i].Width.SetValue(m_uWidth);
			m_bsCameras[i].Height.SetValue(m_uHeight);
            m_bsCameras[i].ExposureTime.SetValue(m_fExposureTime);
            m_bsCameras[i].GainSelector.SetValue(GainSelector_All);
            m_bsCameras[i].Gain.SetValue(m_fGain);

            if (m_bsCameras[i].BslPeriodicSignalSource.GetValue() != BslPeriodicSignalSource_PtpClock ){
               printf("Clock source of periodic signal is not `PtpClock`\n");
               return 0;
            }
            // cout<<"ptp clock:"<<m_bsCameras[i].BslPeriodicSignalSource.GetValue()<<endl;

            m_bsCameras[i].BslPeriodicSignalPeriod.SetValue(1/m_fAcquisitionFrameRate  * 1e6);
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
    
   

    for (unsigned int i = 0; i < m_uDeviceNum; i++)
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
    // m_bsCameras.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    m_tGrabThread = std::make_unique<std::thread>(std::bind(&BaslerMultipleCameras::ThreadSingleGrabFun, this));
    // for (unsigned int i = 0; i < m_uDeviceNum; i++)
    // {
        

    //     m_tGrabThreads.push_back(std::make_unique<std::thread>(std::bind(&BaslerMultipleCameras::ThreadGrabFun, this, i)));
    //     if (i == 0 )
    //         printf("Grabbing will start %d seconds later!\n", duraSec);
    //     if (m_tGrabThreads[i] == nullptr)
    //     {
    //         printf("Create grab thread fail! DevIndex[%d]. Exiting...\r\n", i);
    //         return -1;
    //     }
        
    // }
    
    
    
    // int c_countOfImagesToGrab = 200;
    // for (uint32_t i = 0; i < c_countOfImagesToGrab && m_bsCameras.IsGrabbing(); i++) 
    // {
    //     // This smart pointer will receive the grab result data.
    //     CBaslerUniversalGrabResultPtr ptrGrabResult;
    //      const int DefaultTimeout_ms = 5000;
    //     for (size_t j = 0; j < m_uDeviceNum && m_bsCameras.IsGrabbing(); ++j)
    //     {
    //             // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
    //             m_bsCameras.RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
    //             // std::cout<<"ptrGrabResult:"<<ptrGrabResult->GrabSucceeded()<<std::endl;
    //             // When the cameras in the array are created the camera context value
    //             // is set to the index of the camera in the array.
    //             // The camera context is a user-settable value.
    //             // intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
    //             // if (ptrGrabResult->GrabSucceeded())
    //             // {
    //             //     std::cout<<cameraIndex<<". Cam, Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;

    //             //     // std::cout<<"Timestamp:"<<ptrGrabResult->GetTimeStamp()<<std::endl;
    //             //     //     // Print the index and the model name of the camera.
    //             //     // std::cout << "Camera " << cameraIndex << ": " << m_bsCameras[cameraIndex].GetDeviceInfo().GetModelName() <<
    //             //     //     " (" << m_bsCameras[cameraIndex].GetDeviceInfo().GetIpAddress() << ")" << std::endl;

    //             //     // // You could process the image here by accessing the image buffer.
    //             //     // std::cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << std::endl;
    //             //     const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
    //             //     // std::cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << std::endl << std::endl;
    //             //     std::string filename= "GrabbedImage_" + std::to_string (i) + ".tiff";
    //             //     String_t file_name(filename.c_str());
    //             //     // std::cout<<file_name<<std::endl;
    //             //     // CImagePersistence::Save( ImageFileFormat_Tiff, file_name, ptrGrabResult );
    //             // }
    //             // else
    //             // {
    //             //     // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
    //             //     // multiple images simultaneously. See note above c_maxCamerasToUse.
    //             //     std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
    //             // }
    //             // ptrGrabResult.Release();
    
    //     }
    
    // }

   
    
   
   return m_nExitCode;

}

// Thread function for Single grab
int BaslerMultipleCameras::ThreadSingleGrabFun (){

    unsigned i = 0;
    m_bsCameras.StartGrabbing();

    while(m_bsCameras.IsGrabbing())
    {
        std::cout<<"counter:"<<++i<<std::endl;
        // This smart pointer will receive the grab result data.
        if (m_bExit) 
            break;
        CBaslerUniversalGrabResultPtr ptrGrabResult;
         const int DefaultTimeout_ms = 5000;
        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {
                // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
                m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
                // if (ptrGrabResult->GrabSucceeded())
                // {
                //     // std::cout<<cameraIndex<<". Cam, Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;

                //     // std::cout<<"Timestamp:"<<ptrGrabResult->GetTimeStamp()<<std::endl;
                //     //     // Print the index and the model name of the camera.
                //     // std::cout << "Camera " << cameraIndex << ": " << m_bsCameras[cameraIndex].GetDeviceInfo().GetModelName() <<
                //     //     " (" << m_bsCameras[cameraIndex].GetDeviceInfo().GetIpAddress() << ")" << std::endl;

                //     // // // You could process the image here by accessing the image buffer.
                //     // // std::cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << std::endl;
                //     // const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                //     // // std::cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << std::endl << std::endl;
                //     // std::string filename= "GrabbedImage_" + std::to_string (i) + ".tiff";
                //     // String_t file_name(filename.c_str());
                //     // // std::cout<<file_name<<std::endl;
                //     // // CImagePersistence::Save( ImageFileFormat_Tiff, file_name, ptrGrabResult );


                //     u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();

                //     // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
                //     uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                //     size_t bufferSize = ptrGrabResult->GetBufferSize();
                //     int cameraIndex = ptrGrabResult->GetCameraContext();
                //     size_t cameraPadding = ptrGrabResult->GetPaddingX();
                //     // std::cout<<"cameraPadding:"<<cameraPadding<< " "<<ptrGrabResult->GetWidth()<<std::endl;
                

                //     //std::shared_ptr<uint8_t[]>  tmpSharedptr(pImageBuffer);
                //     // std::unique_ptr<uint8_t[]>  tmpUniqueptr (pImageBuffer);
                //     std::shared_ptr<uint8_t[]>  tmpUniqueptr (new uint8_t[bufferSize]);
                //     memcpy(tmpUniqueptr.get(), pImageBuffer, bufferSize);
                //     DATA data{tmpUniqueptr, timeStamp, bufferSize};
                //     m_queueGrabRes[cameraIndex].push_back(data);
                // }
                // else
                // {
                //     // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
                //     // multiple images simultaneously. See note above c_maxCamerasToUse.
                //     std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
                // }
                // ptrGrabResult.Release();




         }


    }

    return m_nExitCode;
}



// Stop grabbing
int BaslerMultipleCameras::StopGrabbing()
{
  
   
  
    m_tGrabThread->join();

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (unsigned int  i = 0; i < m_uDeviceNum; i++)
    {
       
            // m_tGrabThreads[i]->join();
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



