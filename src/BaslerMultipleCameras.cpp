
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <functional> 
#include <cmath>  
#include <time.h>
#include <memory>
#include <cstdint>
#include <exception>
#include <numeric>
// #include <Windows.h>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/bind/bind.hpp>
// #include <opencv2/opencv.hpp>

#include "BaslerMultipleCameras.h"
#include "SafeVector.h"
#include "magic_enum.hpp"
// #include "memcopy_func.h"


#include "tiffio.h"

#ifdef DEBUG
#ifdef _MSC_VER 
#define DEBUG_PRINT(...) printf_s(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#endif
#else
#define DEBUG_PRINT(...) do {} while (0)
#endif

// #define ENSURE(expr) do { if (expr) break; std::printf("Error: %s\n", #expr); std::abort(); } while (false)

// FBS Calculator
thread_local unsigned count = 0;
thread_local double last = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
thread_local double fps = 0.0;
thread_local bool starter = false;
#define FPS_CALC(_WHAT_, ncurrCameraIndex, allow_print) \
do \
{ \
    double now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); \
    ++count; \
    if (now - last >= 2.0) \
    { \
      std::cerr << "\033[1;31m";\
      fps = double(count)/double(now - last) ; \
      if (allow_print) \
        std::cerr << ncurrCameraIndex<< ". Camera,"<<" Average framerate("<< _WHAT_ << "): " << fps << " fps." <<  "\n"; \
      std::cerr << "\033[0m";\
      count = 0; \
      last = now; \
    } \
} while(false)

 
class CBaslerImageEventHandler : public CImageEventHandler
{
public:
    using  SafeQueueRef = std::vector<std::queue<DATA>>  &;
    using condVectorRef = std::vector<std::condition_variable> &;
    

    CBaslerImageEventHandler(SafeQueueRef  varSafeQueueVec, std::vector<std::mutex> &mutex_vec, condVectorRef cond_vec, std::vector<unsigned int> &loss_vecs, std::vector<unsigned int> &tot_img_vecs /*, std::vector<std::unique_ptr<npp::ImageNPP_8u_C1>> &bayer_srcs, std::vector<std::unique_ptr<npp::ImageNPP_8u_C4>> &rgba_dsts,  std::vector<NppStreamContext> & npp_stream_conts*/):
    var_safe_queueVec_(varSafeQueueVec),
    cond_vec_(cond_vec),
    mutex_vec_(mutex_vec),
    loss_vecs_(loss_vecs),
    tot_img_vecs_(tot_img_vecs)
   
    
    {
        for (unsigned int i = 0; i < loss_vecs_.size() ; i++) {
        
            reset_flag.push_back(false);
        }
      
    };

    virtual void OnImagesSkipped(CInstantCamera& camera, int countOfSkippedImages) {
        printf("OnImagesSkipped event for device %s ", camera.GetDeviceInfo().GetModelName().c_str());
        printf("# of images have been skipped: %d",countOfSkippedImages);
    }

    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult )
    {
         int cameraIndex = ptrGrabResult->GetCameraContext();
      
        tot_img_vecs_[cameraIndex]++;
       
           
        
           
        if (!ptrGrabResult->GrabSucceeded()) {

          
            loss_vecs_[cameraIndex]++;
           std::cout<<"loss vecs: "<<cameraIndex<<" "<<loss_vecs_[cameraIndex]<<std::endl;
            
        } else
        {
            u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();

            // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
            uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            size_t bufferSize = ptrGrabResult->GetBufferSize();
           
            size_t cameraPadding = ptrGrabResult->GetPaddingX();
            int width = ptrGrabResult->GetWidth();
            int height = ptrGrabResult->GetHeight();

           
            uint8_t *tmpBuffer = new uint8_t[bufferSize];// (u_int8_t*)malloc(width* height);
            std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpBuffer);
          
            const std::string serialNumber{camera.GetDeviceInfo().GetSerialNumber().c_str()};
          
            {

                std::lock_guard<std::mutex> lock(mutex_vec_[cameraIndex]);
            
                DATA data = {tmpBuffer, timeStamp, bufferSize, serialNumber};
                var_safe_queueVec_[cameraIndex].push( data);

            }
            cond_vec_[cameraIndex].notify_one();

         
          
        }

       
    }

   
    private :
        SafeQueueRef var_safe_queueVec_;
        condVectorRef &cond_vec_;
        std::vector<std::mutex> &mutex_vec_;
        std::vector<unsigned int> &loss_vecs_;
        std::vector<unsigned int> &tot_img_vecs_;
        std::vector<bool> reset_flag;
      
};




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
      std::cerr <<  ncurrCameraIndex<< ". Camera,"<<" Average framerate("<< _WHAT_ << "): " << double(count_buf)/double(now_buf - last_buf) << " Hz. Queue size: " << buff[ncurrCameraIndex].size() << " Frame Number: "<<counter <<"\n"; \
      count_buf = 0; \
      last_buf = now_buf; \
    } \
}while(false)


#define FPS_CALC_BUF(_WHAT_, buff) \
do \
{ \
    static unsigned count_buf = 0;\
    static unsigned counter = 0; \
    static double last_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();\
    double now_buf = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count(); \
    ++count_buf; \
    ++counter; \
    if (now_buf - last_buf >= 2.0) \
    { \
      std::cerr << "\033[1;31m";\
      std::cerr << "Average framerate("<< _WHAT_ << "): " << double(count_buf)/double(now_buf - last_buf) << " Hz. Queue size: " << buff.size () << " Frame Number: "<<counter <<"\n"; \
      std::cerr << "\033[0m";\
      count_buf = 0; \
      last_buf = now_buf; \
    } \
}while(false)


// BaslerMultipleCameras dialog
BaslerMultipleCameras::BaslerMultipleCameras( const std::string& cameraSettingsFile):
     
      m_tlFactory (CTlFactory::GetInstance())
    , m_sCameraSettingsFile(cameraSettingsFile)



    
   
{
    EnumDevices();
  
    if (m_uDeviceNum > 0)
    {
        m_bsCameras.Initialize(m_uDeviceNum);
        m_queueGrabRes.resize(m_uDeviceNum);
     

        m_mProduceConsumeMutexes_= std::vector<std::mutex>(m_uDeviceNum);
        m_cProduceConsumeConds_ = condVector(m_uDeviceNum);
        m_uLossRatioVec_.resize(m_uDeviceNum, 0);
        m_uTotalNumImgVec_.resize(m_uDeviceNum, 0);
        m_bStarters_.resize(m_uDeviceNum, false);
      
        m_Barrier_.initialize(m_uDeviceNum);
        
        ck(cuInit(0));

        int nGpu = 0;
		ck(cuDeviceGetCount(&nGpu));
        // nGpu =1;
		
        std::cout<<"Number of GPUs:"<<nGpu<<std::endl;
        cu_contexts_.resize(nGpu, nullptr);
        cuDevices_.resize(nGpu, 0);
        for (int iGpu = 0; iGpu <nGpu; iGpu++ ) {
            ck(cuDeviceGet(&cuDevices_[iGpu], iGpu));
            char szDeviceName[80];
            ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevices_[iGpu]));
            std::cout << "GPU in use: " << szDeviceName << std::endl;
            ck(cuCtxCreate(&cu_contexts_[iGpu], 0, cuDevices_[iGpu]));
        }

        

     


    } 
    else 
    {
        fprintf(stderr, "Exiting...\n");
        exit(0);

    }
    
}


 BaslerMultipleCameras::~BaslerMultipleCameras(){
  
   
    CloseDevices();
   
}	   

// Thread Function for save images on disk for every camera
int BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun(int nCurCameraIndex)
{
        unsigned int i =0;
        
        
        
       
        
        unsigned int sep_cam_num =  std::ceil(m_uDeviceNum*19.0/24);
        while(true) {
              
                  
                if ( m_bExit ) {
                    m_bGrabExitFlag = true;
                    m_soundCond_.notify_one();
                    break; 
                }

                
                std::unique_lock<std::mutex> lock(m_mProduceConsumeMutexes_[nCurCameraIndex]);
               
                m_cProduceConsumeConds_[nCurCameraIndex].wait(lock, [&] {
                    return !m_queueGrabRes[nCurCameraIndex].empty();
                });
               

                DATA buff_item = m_queueGrabRes[nCurCameraIndex].front();
                m_queueGrabRes[nCurCameraIndex].pop();
               
                void *device = cu_contexts_[nCurCameraIndex/sep_cam_num];
                CUDA_DRVAPI_CALL(cuCtxSetCurrent((CUcontext)device));
                bayer_device_srcs_[nCurCameraIndex]->copyFrom(buff_item.image, bayer_device_srcs_[nCurCameraIndex]->pitch());


                lock.unlock();
                free(buff_item.image);
                buff_item.image = NULL;
                if (m_iResizeFactor_ != 1.0f && i == m_uFullResCntLimit ) {
                   converter->initializeResize(nCurCameraIndex);

               } 

              
               converter->EncodeCudaFromDevice(bayer_device_srcs_[nCurCameraIndex], nCurCameraIndex, buff_item.timeStamp/1e6,  false, i );
               


                i++;
               
         }
        while (m_queueGrabRes[nCurCameraIndex].size() != 0 ) {

            DATA buff_item = m_queueGrabRes[nCurCameraIndex].front();
            m_queueGrabRes[nCurCameraIndex].pop();
            void *device = cu_contexts_[nCurCameraIndex/sep_cam_num];
            CUDA_DRVAPI_CALL(cuCtxSetCurrent((CUcontext)device));

            bayer_device_srcs_[nCurCameraIndex]->copyFrom(buff_item.image, bayer_device_srcs_[nCurCameraIndex]->pitch());

            free(buff_item.image);
            buff_item.image = NULL;
            if (m_queueGrabRes[nCurCameraIndex].size() == 0) 
                converter->EncodeCudaFromDevice(bayer_device_srcs_[nCurCameraIndex], nCurCameraIndex, buff_item.timeStamp/1e6, true);
            else  
                converter->EncodeCudaFromDevice(bayer_device_srcs_[nCurCameraIndex], nCurCameraIndex, buff_item.timeStamp/1e6, false);
             

             i++;
        }








        
    
    return 0;
}

//Thread function with GetImageBuffer API
int BaslerMultipleCameras::ThreadMultiGrabFun(int nCurCameraIndex)
{
 
    m_bsCameras[nCurCameraIndex].StartGrabbing(m_uFrameNum, GrabStrategy_OneByOne, GrabLoop_ProvidedByUser);
    unsigned int i = 0;
    const int DefaultTimeout_ms = 500000;
   
   
    CBaslerUniversalGrabResultPtr ptrGrabResult;
    while(/*!m_bExit.load(std::memory_order_acquire)*/ !m_bExit && m_bsCameras[nCurCameraIndex].IsGrabbing() )    {
        // if (m_bStarter_.load(std::memory_order_acquire))
            m_uTotalNumImgVec_[nCurCameraIndex]++;

        m_bsCameras[nCurCameraIndex].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
        intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
        if (ptrGrabResult->GrabSucceeded())
        {
           
           
            uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            size_t bufferSize = ptrGrabResult->GetBufferSize();
            u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();
            const std::string serialNumber{m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetSerialNumber().c_str()};
            bool is_starting = m_bStarter_.load(std::memory_order_acquire);
            FPS_CALC("Grabbing Buffer FPS",  nCurCameraIndex, is_starting );
            if (!is_starting) {
                if ( !m_bStarters_[nCurCameraIndex]) {
                    if ((fps > m_fAcquisitionFrameRate - 0.5 && fps < m_fAcquisitionFrameRate + 0.5) ) {
                        m_bStarters_[nCurCameraIndex] = true;

                    }  
                    else {                    
                        if (i % 50==0)
                            std::cerr<<"Cannot start grabbing yet. It is because "<< nCurCameraIndex<< ".Cam fps is not around 30!"<<std::endl;
                        i++;
                        continue; 
                    }
              
                } 
                
                
                if ( nCurCameraIndex == 0 ) {
                    m_bStarter_.store(std::all_of(m_bStarters_.begin(), m_bStarters_.end(), [](bool v) { return v; }), std::memory_order_release);
                    if (!m_bStarter_.load(std::memory_order_acquire)) {
                        // if (i % 10==0)
                        //     std::cerr<<"Cannot start grabbing yet. It is because not all fps's are around 30!"<<std::endl;
                        // i++;
                        continue;
                    }
                    else {
                        std::cerr<<"******* Grabbing just started! ************\n\n\n\n\n\n\n"<<std::endl;
                        m_soundCond_.notify_one();

                    }

                } else continue;
            



            } 

           {
        

                std::lock_guard<std::mutex> lock(m_mProduceConsumeMutexes_[nCurCameraIndex]);
                // clock_t start = clock();
                // std::cout<<"buffer size in grab:"<<bufferSize<<std::endl;
            
            
                uint8_t *tmpBuffer =  new uint8_t[bufferSize];// (u_int8_t*)malloc(width* height);
                std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpBuffer);
                
                
                DATA data{tmpBuffer, timeStamp, bufferSize, serialNumber};
                m_queueGrabRes[nCurCameraIndex].push(data);

            
            }
           
            m_cProduceConsumeConds_[nCurCameraIndex].notify_one();
           
        }
        else
        {
            // if (m_bStarter_.load(std::memory_order_acquire))
                m_uLossRatioVec_[nCurCameraIndex]++;
            
            

           std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << std::endl;//" " << ptrGrabResult->GetErrorDescription() << std::endl;
           if (i%100 == 0 && m_uLossRatioVec_[nCurCameraIndex]/float(m_uTotalNumImgVec_[nCurCameraIndex]) > 0.005)
                throw std::runtime_error("Exception ! Loss Ratio is less than 0.003. Exiting");
        }
        i++;


        


         ptrGrabResult.Release();

      

   
    } 
    

    

    return 0;
}




// Thread function with GetOneFrameTimeOut API

// bool BaslerMultipleCameras::SaveBayerAsTiff(const std::string &file_name, uint8_t *buffer,  uint32_t width, u_int32_t height)
// {
    
//     cv::Mat bayer_image(height, width, CV_8UC1, buffer);
//     cv::Mat rgb_image;
//     cv::cvtColor(bayer_image, rgb_image, cv::COLOR_BayerRG2BGR);

//     TIFF* tif = TIFFOpen(file_name.c_str(), "w");
//     if (tif) {
//         TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
//         TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
//         TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);  // Bayer8 image
//         TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
//         TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
//         TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//         TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);  // or PHOTOMETRIC_MINISWHITE

//         for (int row = 0; row < height; row++) {
//             TIFFWriteScanline(tif, rgb_image.ptr(row), row);
//         }

//         TIFFClose(tif);
//     } else return false;

    
//     return true;
// }

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
      
    }
    catch (const GenericException& e)
    {
        PYLON_UNUSED( e );

        std::cerr<<e.GetDescription()<<std::endl;
    }
    m_uDeviceNum = 1;//m_allDeviceInfos.size() ;
    std::cout<<m_uDeviceNum<<" GigE Cameras Found!"<<std::endl;
    for (unsigned int i = 0; i < m_uDeviceNum; i++) {
        std::cout<<i<<".Cam Serial Num:"<<m_allDeviceInfos[i].GetSerialNumber().c_str()<<std::endl;
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
    srand( (unsigned) time( NULL ) );
    m_iDeviceKey = 4711 ;//rand();
    m_iGroupKey = 1;
    m_iAllGroupMask =  0xffffffff ; 
    
    for (unsigned int  i = 0; i < m_uDeviceNum; i++)
    {
        m_tOpenDevicesThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadOpenDevicesFun, this, i)));
       
    }
    for ( auto &th :m_tOpenDevicesThreads)
    {
        if (th.joinable())
            th.join();
    }
    return m_nExitCode;


}

int BaslerMultipleCameras::ThreadOpenDevicesFun(int nCurCameraIndex) 
{
    try
    {
           

           
            m_bsCameras[nCurCameraIndex].Attach( m_tlFactory.CreateDevice( m_allDeviceInfos[nCurCameraIndex] ) );
            // m_bsCameras[nCurCameraIndex].RegisterConfiguration( new CActionTriggerConfiguration( m_iDeviceKey, m_iGroupKey, m_iAllGroupMask ), RegistrationMode_Append, Cleanup_Delete );

            m_bsCameras[nCurCameraIndex].SetCameraContext(nCurCameraIndex );
            // m_bsCameras[nCurCameraIndex].RegisterImageEventHandler( new CBaslerImageEventHandler(m_queueGrabRes , m_mProduceConsumeMutexes_, m_cProduceConsumeConds_, m_uLossRatioVec_, m_uTotalNumImgVec_/*, bayer_device_srcs_, rgba_device_dsts_, npp_stream_contextes_*/), RegistrationMode_Append, Cleanup_Delete );
            m_bsCameras[nCurCameraIndex].GrabCameraEvents = true;

            m_bsCameras[nCurCameraIndex].Open();
              // Check if the device supports events.
            if (!m_bsCameras[nCurCameraIndex].EventSelector.IsWritable())
            {
                throw RUNTIME_EXCEPTION( "The device doesn't support events." );
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
    m_uHeight = pt.get<unsigned int>("Height");
    m_uWidth = pt.get<unsigned int>("Width");
    m_iResizeFactor_= pt.get<float>("ResizeFactor");
    m_uFullResCntLimit =  pt.get<int>("FullResCountLimit");
    m_fExposureTime = pt.get<float>("ExposureTime");
    m_fAcquisitionFrameRate = pt.get<float>("AcquisitionFrameRate");
    m_fGain = pt.get<float>("Gain");
    m_uFrameNum = pt.get<unsigned int>("FrameNum");
    m_sPixelFormat = pt.get<std::string>("PixelFormat");
    m_uPacketSize =  pt.get<unsigned int>("PacketSize");
    m_uPacketDelay = pt.get<unsigned int>("PacketDelay");
    
    auto pixelFormatEnum = magic_enum::enum_cast<Basler_UniversalCameraParams::PixelFormatEnums>(m_sPixelFormat);
  

    
    file.close();
    unsigned int sep_cam_num =  std::ceil(m_uDeviceNum*19.0/24);

    for (unsigned int i = 0 ; i < m_uDeviceNum; i++)
    {
        // ck(cuCtxCreate(&cu_contexts_[i], 0, cuDevices_[i/sep_cam_num]));

        cuCtxPushCurrent(cu_contexts_[i/sep_cam_num]);

        bayer_device_srcs_.push_back( std::make_unique<npp::ImageNPP_8u_C1>(m_uWidth, m_uHeight, true));
        // bayer_device_srcs_resized_.push_back( std::make_unique<npp::ImageNPP_8u_C1>(m_uWidth/2, m_uHeight/2, true));

        cuCtxPopCurrent(nullptr); 
    }

   



    m_timePoint_ = std::chrono::system_clock::now();
    converter = std::make_unique<BayerToH264ConverterNvidiaCodec>(cu_contexts_,  m_mapSerials, m_uDeviceNum, m_uWidth, m_uHeight, (unsigned int)m_fAcquisitionFrameRate, m_iResizeFactor_, m_uFullResCntLimit, m_timePoint_);   
  



    try
    {
        for (size_t i = 0; i < m_uDeviceNum; ++i)
        {
            
           
            m_bsCameras[i].UserSetSelector.SetValue("Default");
            m_bsCameras[i].UserSetLoad.Execute();

          
            if (pixelFormatEnum.has_value())
                m_bsCameras[i].PixelFormat.SetValue(pixelFormatEnum.value());
            else 
                std::cerr << "Pixel Format not set! Continueing with the default or previous value." << std::endl;

           
            while (m_bsCameras[i].GevSCPSPacketSize.GetValue()!= m_uPacketSize) {
				m_bsCameras[i].GevSCPSPacketSize.SetValue(m_uPacketSize);
			}
            m_bsCameras[i].GevSCPD.SetValue(m_uPacketDelay);
            m_bsCameras[i].Width.SetValue(m_uWidth);
			m_bsCameras[i].Height.SetValue(m_uHeight);
            m_bsCameras[i].ExposureTime.SetValue(m_fExposureTime);
            m_bsCameras[i].GainSelector.SetValue(GainSelector_All);
            m_bsCameras[i].Gain.SetValue(m_fGain);

         
            
            // // cout<<"ptp clock:"<<m_bsCameras[i].BslPeriodicSignalSource.GetValue()<<endl;
            // m_bsCameras[i].PtpEnable.SetValue(false);
            // m_bsCameras[i].BslPtpPriority1.SetValue(128);
            // // Enable end-to-end delay measurement
            // m_bsCameras[i].BslPtpProfile.SetValue(BslPtpProfile_DelayRequestResponseDefaultProfile);
            // // Set the network mode to unicast
            // m_bsCameras[i].BslPtpNetworkMode.SetValue(BslPtpNetworkMode_Multicast);
            // m_bsCameras[i].BslPtpManagementEnable.SetValue(true);
            // // Disable two-step operation
            // m_bsCameras[i].BslPtpTwoStep.SetValue(false);
            m_bsCameras[i].PtpEnable.SetValue(true);

           

            // std::cout<<i<<".cam IEEE1588 status:"<<m_bsCameras[i].PtpStatus.GetValue()<< " "<< m_bsCameras[i].TriggerSource.GetValue()<<std::endl;
           
            m_bsCameras[i].BslPeriodicSignalPeriod.SetValue(1/m_fAcquisitionFrameRate  * 1e6);
            m_bsCameras[i].BslPeriodicSignalDelay.SetValue(0);
            m_bsCameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
            m_bsCameras[i].TriggerMode.SetValue(TriggerMode_On);
            m_bsCameras[i].TriggerSource.SetValue(TriggerSource_PeriodicSignal1);
            if (m_bsCameras[i].BslPeriodicSignalSource.GetValue() != BslPeriodicSignalSource_PtpClock ){
               printf("Clock source of periodic signal is not `PtpClock`\n");
               return 0;
            }

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
        // for (int i = 0 ; i <m_uDeviceNum ; i++)
        //     m_bsCameras[i].Close();
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
      
        m_tConsumeThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun, this, i)));
     
    }

 
    return m_nExitCode;

}   





// Start grabbing
int BaslerMultipleCameras::StartGrabbing()
{
    auto now = std::chrono::system_clock::now();
    unsigned int duraSec = 1;
    std::chrono::seconds duration(duraSec);

    m_tWakeupTime = now + duration;
   
    
    for (unsigned int i = 0; i < m_uDeviceNum; i++)
    {

        m_tGrabThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadMultiGrabFun, this, i)));
        std::cout<<i<<".Cam Grab Func just started!"<<std::endl;
              
    }


    for (unsigned int i = 0; i < m_uDeviceNum; i++)
    {
      
        m_tConsumeThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun, this, i)));
     
    }

    
    return m_nExitCode;
  

}

int BaslerMultipleCameras::recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    AudioData *data = (AudioData*)userData;
    const float *in = (const float*)inputBuffer;
    
   
    if (data->isRecording) {
      for (unsigned long i = 0; i < framesPerBuffer; i++) {
          AudioSample sample;
          sample.leftSample = in[i * NUM_CHANNELS];     // Left channel
          sample.rightSample = in[i * NUM_CHANNELS + 1]; // Right channel

          // Calculate timestamp based on initial timestamp and sample count
          auto sampleOffset = std::chrono::milliseconds(
              static_cast<long long>(1000000.0 * data->sampleCount / SAMPLE_RATE));
          sample.timestamp = data->initialTimestamp + sampleOffset;

          data->recordedSamples.push_back(sample);
          data->sampleCount++;
      }
  }

  return paContinue;
    
}

// Thread function for Single grab
int BaslerMultipleCameras::ThreadSingleGrabFun (){

    unsigned i = 0;
    float sum_elapsed = 0.0f;
    // u_int8_t *tmpBuffer = (u_int8_t*)malloc(m_uHeight* m_uWidth);
    // std::vector<CBaslerUniversalGrabResultPtr> ptrGrabResults(m_uDeviceNum);
    // std::vector<std::vector<CBaslerUniversalGrabResultPtr>>  allPtrGrabResults;
    std::vector< std::future<void> > results;//(m_uDeviceNum);
    m_bsCameras.StartGrabbing(GrabStrategy_OneByOne,  GrabLoop_ProvidedByUser);
    while(!m_bGrabExitFlag && m_bsCameras.IsGrabbing() )
    {
       
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;
        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {

            
            m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );

            CBaslerUniversalGrabResultPtr &ptrGrabResult = ptrGrabResult;
            if (ptrGrabResult->GrabSucceeded())
            {
               
                
                u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();

                // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
                uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();

                size_t bufferSize = ptrGrabResult->GetBufferSize();
                int cameraIndex = ptrGrabResult->GetCameraContext();
                size_t cameraPadding = ptrGrabResult->GetPaddingX();
                const std::string serialNumber(m_bsCameras[j].GetDeviceInfo().GetSerialNumber().c_str());

                u_int8_t *tmpBuffer = (u_int8_t*)malloc(bufferSize);

              

                memcpy(tmpBuffer, pImageBuffer, bufferSize);
             
                {
                   std::lock_guard<std::mutex> lock(m_mProduceConsumeMutexes_[j]);
                   DATA data{tmpBuffer, timeStamp, bufferSize, serialNumber};
                   m_queueGrabRes[j].push(data);
                }
                
               
                m_cProduceConsumeConds_[j].notify_one();
                // free(tmpBuffer);
                // tmpBuffer = NULL;

            }
            else
            {
                // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
                // multiple images simultaneously. See note above c_maxCamerasToUse.
                std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
            }
        

        }
       
        i++;
      
  

    }
   
    return m_nExitCode;
}

int BaslerMultipleCameras::ThreadSingleGrabFunWithActCommand (){

    m_bsCameras.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByUser);
    m_bsCameras[0].TimestampLatch.Execute();
    uint64_t currentTimestamp = m_bsCameras[0].TimestampLatchValue.GetValue();
        
    int64_t actionTime = currentTimestamp;
    const u_int64_t timeIntervalNs = uint64_t ((1./m_fAcquisitionFrameRate)*1e9);
    while(!m_bGrabExitFlag && m_bsCameras.IsGrabbing() )    {
        std::cout<<"time interval and action time: "<<timeIntervalNs<< " "<<actionTime<<std::endl; 
        actionTime  += timeIntervalNs;
        m_pTL->IssueScheduledActionCommand(m_iDeviceKey, m_iGroupKey, m_iAllGroupMask, actionTime);
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;

        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {
            // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
            m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
            



        }

     //   FPS_CALC_THREAD_BUF("Grabbing Buffer FPS",  m_queueGrabRes, 0);
        


    
    }
    return 0;
}




// Stop grabbing
int BaslerMultipleCameras::StopGrabbing()
{
  
   

  


    // m_tGrabThread.join();
    m_soundThread_.join();


    for (auto &th: m_tGrabThreads)
    {
        if (th.joinable())
            th.join();
    
    }

    
    for (auto &th: m_tConsumeThreads)
    {
        if (th.joinable())
          th.join();
        
    }


   

    float tot_loss =0;
    for(int i = 0; i <m_uDeviceNum;i++){
        std::cout<<i<<" Cam counter val:"<<m_uTotalNumImgVec_[i]<<std::endl;
        std::cout<<i<<". cam, loss frame ratio:"<<float(m_uLossRatioVec_[i])/m_uTotalNumImgVec_[i] <<std::endl;
        tot_loss += float(m_uLossRatioVec_[i])/m_uTotalNumImgVec_[i];

    }
    std::cout<<"Total Loss Frame Ratio:"<<tot_loss/m_uDeviceNum<<std::endl;

   

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

int BaslerMultipleCameras::StartSoundRecording(){
 m_soundThread_ = std::thread(std::bind(&BaslerMultipleCameras::ThreadStartSoundRecording, this));
 return 0;
}


int BaslerMultipleCameras::ThreadStartSoundRecording()
{
    
    

    
    
    
    std::unique_lock<std::mutex> lock(m_soundMutex_);

    // m_soundCond_.wait_for(lock, std::chrono::seconds(5));//==std::cv_status::timeout)
    // {
    //     std::cout << '.' << std::endl;
    // }
    m_soundCond_.wait(lock, [&] { return !m_bExit && m_bStarter_.load(std::memory_order_acquire) ;});

    // m_initTimeStamp_= 16003456723;
    // auto time_stamp_t = std::chrono::system_clock::time_point(
    //     std::chrono::milliseconds(m_initTimeStamp_));

    // auto now =  std::chrono::system_clock::now();
    // m_initTimeStamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::time_t now_t = std::chrono::system_clock::to_time_t(m_timePoint_);
    std::stringstream ss;
        
    ss << std::put_time(std::localtime(&now_t), "%Y-%m-%d_%H-%M-%S");
    std::string folderName = "../recordings/" + ss.str();
    if (mkdir(folderName.c_str(), 0777) == 0 || errno == EEXIST) {
        std::cout << folderName <<" directory created or already exists." << std::endl;
    } else {
        std::cerr << "Failed to create "<< folderName<<" directory." << std::endl;
        return -1;
    }

    std::string soundFileName = folderName + "/recorded_audio.wav";

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // AudioData data;

    m_soundData_.isRecording = false;
    m_soundData_.sampleCount = 0;
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                             NUM_CHANNELS,
                             0,
                             paFloat32,
                             SAMPLE_RATE,
                             FRAMES_PER_BUFFER,
                             recordCallback,
                             &m_soundData_);
    
    if (err != paNoError) {
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

   
    m_soundData_.initialTimestamp = m_timePoint_;//std::chrono::system_clock::time_point(
      //  std::chrono::milliseconds(m_initTimeStamp_));
    m_soundData_.sampleCount = 0;
    m_soundData_.recordedSamples.clear();
    m_soundData_.isRecording = true;

    // Record for a fixed duration, e.g., 10 seconds
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    m_soundCond_.wait(lock, [] { return m_bExit; });
    m_soundData_.isRecording = false;

    std::cout << "Recording stopped. Saving files..." << std::endl;
    saveToWavWithEmbeddedTimestamps(m_soundData_.recordedSamples, soundFileName.c_str());
    std::cout << "Audio saved to 'recorded_audio.wav'" << std::endl;
    
   

    err = Pa_StopStream(stream);
    err = Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;
}
void BaslerMultipleCameras::saveToWavWithEmbeddedTimestamps(const std::vector<AudioSample>& samples, const char* audioFile) {
  std::ofstream file(audioFile, std::ios::binary);

  // Prepare timestamp data
  std::vector<TimestampData> timeData;
  timeData.reserve(samples.size());

  for (size_t i = 0; i < samples.size(); i++) {
      TimestampData td;
      td.sampleIndex = i;
      td.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
          samples[i].timestamp.time_since_epoch()).count();
      timeData.push_back(td);
  }

  // Calculate sizes
  uint32_t timeChunkSize = sizeof(TimestampData) * timeData.size();
  uint32_t dataChunkSize = samples.size() * sizeof(float) * NUM_CHANNELS;

  // Create and write WAV header
  WAVHeader header;
  header.riffSize = sizeof(WAVHeader) - 8 +
                    sizeof(TimeChunkHeader) + timeChunkSize +
                    8 + dataChunkSize;

  file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));

  // Write TIME chunk
  TimeChunkHeader timeHeader;
  timeHeader.timeSize = timeChunkSize;
  file.write(reinterpret_cast<const char*>(&timeHeader), sizeof(TimeChunkHeader));
  file.write(reinterpret_cast<const char*>(timeData.data()), timeChunkSize);

  // Write data chunk header
  file.write("data", 4);
  file.write(reinterpret_cast<const char*>(&dataChunkSize), 4);

  // Write audio samples
  for (const auto& sample : samples) {
      file.write(reinterpret_cast<const char*>(&sample.leftSample), sizeof(float));
      file.write(reinterpret_cast<const char*>(&sample.rightSample), sizeof(float));
  }

  file.close();

  // Save CSV file
  std::string csvFilename = std::string(audioFile) + ".csv";
  std::ofstream csv(csvFilename);
  csv << "Sample Index,Timestamp(microseconds)\n";//,Timestamp (human readable),Left Value,Right Value\n";

  for (size_t i = 0; i < samples.size(); i++) {
      auto timestamp = std::chrono::system_clock::to_time_t(samples[i].timestamp);
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(
          samples[i].timestamp.time_since_epoch()).count();

      std::stringstream ss;
      ss << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S");
    //   std::cout<<ss.str()<<std::endl;
      csv << i << ","
          << us << ".\n";
        //   << ss.str() << "."
        //   << std::setfill('0') << std::setw(6) << (us % 1000000) << ","
        //   << samples[i].leftSample << ","
        //   << samples[i].rightSample << "\n";
  }

  csv.close();
}

// void BaslerMultipleCameras::saveToWavWithEmbeddedTimestamps(const std::vector<AudioSample> &samples, const char *audioFile)
// {
//     std::ofstream file(audioFile, std::ios::binary);
    
//     // Prepare timestamp data
//     std::vector<TimestampData> timeData;
//     timeData.reserve(samples.size());
    
//     for (size_t i = 0; i < samples.size(); i++) {
//         TimestampData td;
//         td.sampleIndex = i;
//         td.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
//             samples[i].timestamp.time_since_epoch()).count();
//         timeData.push_back(td);
//     }

//     // Calculate sizes
//     uint32_t timeChunkSize = sizeof(TimestampData) * timeData.size();
//     uint32_t dataChunkSize = samples.size() * sizeof(float);
    
//     // Create and write WAV header
//     WAVHeader header;
//     header.riffSize = sizeof(WAVHeader) - 8 +
//                       sizeof(TimeChunkHeader) + timeChunkSize +
//                       8 + dataChunkSize;
    
//     file.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
    
//     // Write TIME chunk
//     TimeChunkHeader timeHeader;
//     timeHeader.timeSize = timeChunkSize;
//     file.write(reinterpret_cast<const char*>(&timeHeader), sizeof(TimeChunkHeader));
//     file.write(reinterpret_cast<const char*>(timeData.data()), timeChunkSize);
    
//     // Write data chunk header
//     file.write("data", 4);
//     file.write(reinterpret_cast<const char*>(&dataChunkSize), 4);
    
//     // Write audio samples
//     for (const auto& sample : samples) {
//         file.write(reinterpret_cast<const char*>(&sample.sample), sizeof(float));
//     }
    
//     file.close();
    
//     // Save CSV file
//     std::string csvFilename = std::string(audioFile) + ".csv";
//     std::ofstream csv(csvFilename);
//     csv << "Sample Index,Timestamp (microseconds),Timestamp (human readable),Value\n";
    
//     for (size_t i = 0; i < samples.size(); i++) {
//         auto timestamp = std::chrono::system_clock::to_time_t(samples[i].timestamp);
//         auto us = std::chrono::duration_cast<std::chrono::microseconds>(
//             samples[i].timestamp.time_since_epoch()).count();
        
//         std::stringstream ss;
//         ss << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S");
        
//         csv << i << ","
//             << us << ","
//             << ss.str() << "."
//             << std::setfill('0') << std::setw(6) << (us % 1000000) << ","
//             << samples[i].sample << "\n";
//     }
    
//     csv.close();
// }
