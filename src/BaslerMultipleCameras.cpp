
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
    // rgba_dsts_(rgba_dsts),
    // bayer_srcs_(bayer_srcs),
    // npp_stream_conts_(npp_stream_conts)
    
    {
        for (unsigned int i = 0; i < loss_vecs_.size() ; i++) {
            // bayer_dev_srcs_.push_back(std::make_unique<npp::ImageNPP_8u_C1>(4096,3000, true));
            // rgba_device_dsts_.push_back(std::make_unique<npp::ImageNPP_8u_C4>(4096,3000, true));
            reset_flag.push_back(false);
        }
        // rgba_device_dst = std::make_unique<npp::ImageNPP_8u_C4>(4096,3000, true);
        // tmpBuffer = (u_int8_t*)malloc(size_t(4096* 3000));
    };

    virtual void OnImagesSkipped(CInstantCamera& camera, int countOfSkippedImages) {
        printf("OnImagesSkipped event for device %s ", camera.GetDeviceInfo().GetModelName().c_str());
        printf("# of images have been skipped: %d",countOfSkippedImages);
    }

    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult )
    {
         int cameraIndex = ptrGrabResult->GetCameraContext();
        // if (cameraIndex == 2) tot_cnt++;
        // else if (cameraIndex == 4) tot_cnt2++;
        tot_img_vecs_[cameraIndex]++;
        // if (!reset_flag[cameraIndex] && tot_img_vecs_[cameraIndex] >= 100){
        //     tot_img_vecs_[cameraIndex] = 0;
        //     loss_vecs_[cameraIndex] = 0;
        //     reset_flag[cameraIndex] = true;
            
        // } else if (!reset_flag[cameraIndex]){
        //     // std::cout<<"Returning! "<<cameraIndex<<" "<<tot_img_vecs_[cameraIndex]<<std::endl;
        //     return;
        // }
           
        
           
        if (!ptrGrabResult->GrabSucceeded()) {

            // std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() <<std::dec <<std::endl;//" " << ptrGrabResult->GetErrorDescription() << std::endl;
            // if (cameraIndex == 2) cnt++;
            // else if (cameraIndex == 4) cnt2++;
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

            // npp::ImageNPP_8u_C1 *tmp_bayer = new  npp::ImageNPP_8u_C1(width, height, true);
            // npp::ImageNPP_8u_C4 *tmp_rgba = new npp::ImageNPP_8u_C4(width, height, true);
            //  std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
            uint8_t *tmpBuffer = new uint8_t[bufferSize];// (u_int8_t*)malloc(width* height);
            std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpBuffer);
            // clock_t start1 = clock();
            // bayer_srcs_[cameraIndex]->copyFromAsync(pImageBuffer, bayer_srcs_[cameraIndex]->pitch(), npp_stream_conts_[cameraIndex].hStream);
            // bayer_srcs_[cameraIndex]->copyFrom(pImageBuffer, bayer_srcs_[cameraIndex]->pitch());
            // clock_t end1= clock();
            // double elapsed1 = (double(end1-start1))/ CLOCKS_PER_SEC;
                
            // printf("%d cam, time taken by memcpy in seconds : %f \n", cameraIndex, elapsed);
            // tmp_bayer->copyFromAsync(pImageBuffer, tmp_bayer->pitch());
            // if (tot_img_vecs_[cameraIndex] == 1 &&  loss_vecs_[cameraIndex] == 0) 
            //     bayer_srcs_[cameraIndex]->copyFrom(pImageBuffer, bayer_srcs_[cameraIndex]->pitch());
            // if ( cameraIndex == 0) {
            // clock_t start2 = clock();
            // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R_Ctx(bayer_srcs_[cameraIndex]->data(), (int)bayer_srcs_[cameraIndex]->width(), {(int)bayer_srcs_[cameraIndex]->width(), (int)bayer_srcs_[cameraIndex]->height()}, 
            //             {0, 0, (int)bayer_srcs_[cameraIndex]->width(),(int)bayer_srcs_[cameraIndex]->height() }, (Npp8u *)rgba_dsts_[cameraIndex]->data(), (int)rgba_dsts_[cameraIndex]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100, npp_stream_conts_[cameraIndex]);
            // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(bayer_srcs_[cameraIndex]->data(), (int)bayer_srcs_[cameraIndex]->width(), {(int)bayer_srcs_[cameraIndex]->width(), (int)bayer_srcs_[cameraIndex]->height()}, 
            //             {0, 0, (int)bayer_srcs_[cameraIndex]->width(),(int)bayer_srcs_[cameraIndex]->height() }, (Npp8u *)rgba_dsts_[cameraIndex]->data(), (int)rgba_dsts_[cameraIndex]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100);
            
            // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(tmp_bayer->data(), (int)tmp_bayer->width(), {(int)tmp_bayer->width(), (int)tmp_bayer->height()}, 
            //             {0, 0, (int)tmp_bayer->width(),(int)tmp_bayer->height() }, (Npp8u *)tmp_rgba->data(), (int)tmp_rgba->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100);
            
            
            // cudaError_t cudaResult = cudaStreamSynchronize(npp_stream_conts_[cameraIndex].hStream);
            // ENSURE(cudaSuccess == cudaResult);

            // cudaDeviceSynchronize();

            // mem_cpy(tmpBuffer, pImageBuffer, bufferSize);
            // free(tmpBuffer);
            // tmpBuffer =NULL;
            // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpBuffer);
            // X_aligned_memcpy_sse2(tmpSharedptr.get(), pImageBuffer, bufferSize);
            // clock_t end2= clock();
            
            // double elapsed2 = (double(end2-start2))/ CLOCKS_PER_SEC;
                
            // printf("%d cam, time taken by memcpy in seconds : %f, %f\n", cameraIndex, elapsed1, elapsed2);
            // }
            const std::string serialNumber{camera.GetDeviceInfo().GetSerialNumber().c_str()};
            // delete tmp_bayer;
            // delete tmp_rgba;

            
            // cnt[cameraIndex]++;
            // if (cameraIndex == 0 ) {
            //     sum_elapsed += elapsed;
            //     cnt++;
            // }

            // if (cameraIndex == 5) {
            //     sum_elapsed2 += elapsed;
            //     cnt2++;
            // }
            // std::thread::id this_id = std::this_thread::get_id();
            // std::cout<<"Thread id:"<<this_id<<std::endl;
            // if (cameraIndex == 0  && cnt %100 == 0)
            //     std::cout<<"Average elaped time with memcpy in seconds :"<< sum_elapsed/cnt<<" "<<bufferSize<<std::endl;
            //   if (cameraIndex == 5  && cnt2 %100 == 0)
            //     std::cout<<"Average elaped time 2 with memcpy in seconds :"<< sum_elapsed2/cnt2<<" "<<bufferSize<<std::endl;
            {

                std::lock_guard<std::mutex> lock(mutex_vec_[cameraIndex]);
            
                DATA data = {tmpBuffer, timeStamp, bufferSize, serialNumber};
                var_safe_queueVec_[cameraIndex].push( data);

            }
            cond_vec_[cameraIndex].notify_one();

            // FPS_CALC("in grabbed event handler", cameraIndex);
            

            // DATA data = {tmpBuffer, timeStamp, bufferSize, serialNumber};
            // var_safe_queueVec_[cameraIndex].enqueue( data);
          
        }

       
    }

   
    private :
        SafeQueueRef var_safe_queueVec_;
        condVectorRef &cond_vec_;
        std::vector<std::mutex> &mutex_vec_;
        std::vector<unsigned int> &loss_vecs_;
        std::vector<unsigned int> &tot_img_vecs_;
        std::vector<bool> reset_flag;
        // u_int8_t *tmpBuffer;
        // std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> bayer_dev_srcs_;//(m_uWidth, m_uHeight, true);

        // std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> &bayer_srcs_;
        // std::vector<std::unique_ptr<npp::ImageNPP_8u_C4>> &rgba_dsts_;
        // std::vector<NppStreamContext> &npp_stream_conts_;
};


// std::vector<unsigned int> CBaslerImageEventHandler::cnt = std::vector<unsigned int>(11, 0);
// std::vector<float> CBaslerImageEventHandler::sum_elapsed = std::vector<float>(11, 0.0f);

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
    // const size_t queue_size = 10; // Set the desired size of the queues
    // std::vector<boost::lockfree::queue<int>> queues(queue_size);
    if (m_uDeviceNum > 0)
    {
        m_bsCameras.Initialize(m_uDeviceNum);
        // m_queueGrabRes =  std::vector<boost::lockfree::queue<DATA,  boost::lockfree::capacity<500> >>(m_uDeviceNum);
        m_queueGrabRes.resize(m_uDeviceNum);
        // m_queueGrabRes = std::vector<tbb::concurrent_queue<DATA>>(m_uDeviceNum);
        for (int i = 0; i < m_uDeviceNum; i++) {
            
            // m_queueGrabRes.push_back(moodycamel::ConcurrentQueue<DATA>());
            // m_queueGrabRes.emplace_back(boost::lockfree::queue<DATA>());
        // converter->initializeContexts("AllCameras", m_mapSerials);
        }
        // outfile.open("bayer8_2.bin", std::ios::binary);
        m_threadPool.reset(m_uDeviceNum);

        m_mProduceConsumeMutexes_= std::vector<std::mutex>(m_uDeviceNum);
        m_cProduceConsumeConds_ = condVector(m_uDeviceNum);
        m_uLossRatioVec_.resize(m_uDeviceNum, 0);
        m_uTotalNumImgVec_.resize(m_uDeviceNum, 0);
        vec_data_struct.resize(m_uDeviceNum);
        // cuda_streams_.resize(m_uDeviceNum, nullptr);
        // npp_stream_contextes_.resize(m_uDeviceNum, {});
        // for (int i = 0; i < m_uDeviceNum; i++) {
        //     cudaError_t cudaResult = cudaStreamCreateWithFlags(&cuda_streams_[i], cudaStreamNonBlocking);
        //     ENSURE(cudaSuccess == cudaResult);

        //     // Create an NPP stream context that uses this stream.
        //     NppStatus nppStatus = nppGetStreamContext(&npp_stream_contextes_[i]);
        //     ENSURE(NPP_SUCCESS == nppStatus);
        //     npp_stream_contextes_[i].hStream = cuda_streams_[i];
        // }

        // m_cEndGrabConds_ = condVector(m_uDeviceNum);

        
    
    } 
    else 
    {
        fprintf(stderr, "Exiting...\n");
        exit(0);

    }
    
}


 BaslerMultipleCameras::~BaslerMultipleCameras(){
    // for (auto &st: npp_stream_contextes_)
    //     cudaStreamDestroy(st.hStream);
    //     // outfile.close();
    }	   

// Thread Function for save images on disk for every camera
int BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun(int nCurCameraIndex)
{
        unsigned int i =0;

        // std::this_thread::sleep_for(std::chrono::milliseconds(300));
        // // npp::ImageNPP_8u_C1 bayer_device_src(m_uWidth, m_uHeight, true);
        // // npp::ImageNPP_8u_C4 rgb_device_dst(m_uWidth, m_uHeight, true);

        // u_int8_t *tmpBuffer = (u_int8_t*)malloc(m_uHeight* m_uWidth);
        // unsigned int i =0;
        // while(true) {
        //         // if (m_queueGrabRes[nCurCameraIndex].empty())
        //         //     break;
        //         // std::cout<<"m_bExit:"<<m_bExit<<std::endl;

        //         if (m_bExit) {
        //             m_bGrabExitFlag = true;
        //             // m_bGrabExitFlag.store(true, std::memory_order_release);
        //             // std::cout<<"m_grab_flag 2:"<<m_bGrabExitFlag<<std::endl;

        //             break; 
        //         }
        //         std::unique_lock<std::mutex> lock(m_mProduceConsumeMutexes_[nCurCameraIndex]);
        //         // while ( /*!m_bExit.load(std::memory_order_acquire) || */ m_queueGrabRes[nCurCameraIndex].empty()){
        //         //     m_cProduceConsumeConds_[nCurCameraIndex].wait(lock);
        //         // }

        //         m_cProduceConsumeConds_[nCurCameraIndex].wait(lock, [&] {
        //             return !m_queueGrabRes[nCurCameraIndex].empty();
        //         });
        //         // DATA buff_item = m_queueGrabRes[nCurCameraIndex].dequeue();
        //         lock.unlock();
        //         // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        //         // 
        //         // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

                
        //         // m_bExit.store(true, std::memory_order_release);
        //         // m_bsCameras[nCurCameraIndex].StopGrabbing();

        //         // break;

        //         DATA buff_item = m_queueGrabRes[nCurCameraIndex].front();
        //         m_queueGrabRes[nCurCameraIndex].pop();
        //         // free(buff_item.image);
        //         buff_item.image = NULL;
                

        //         std::string filename= "/mnt/m2_storage/test_images/GrabbedImage_" + std::to_string (buff_item.timeStamp/1000000) + "_" + std::to_string (nCurCameraIndex) + ".tiff";
        //         String_t file_name(filename.c_str());
        //         // std::cout<<filename<<std::endl;
        //         std::ofstream outfile (filename);
        //         // outfile.write(reinterpret_cast<char*>(tmpBuffer), imageSize);
        //         outfile.close();
         
             
              
        //         // continue;

        //     //     int imageSize = buff_item.imageSize;
        //     //  //  
        //         // std::copy(buff_item.image, buff_item.image + imageSize, tmpBuffer);
        //     //    tmpBuffer = nullptr;
        //     //    tmpBuffer = buff_item.image;
        //         // 
                 
                
             
           
         

        //         // std::copy(buff_item.image, buff_item.image + imageSize, tmpBuffer);
        //         // buff_item.image.reset();
        //         // free(buff_item.image);
        //         // buff_item.image = NULL;
           
        //         // memset(tmpBuffer, 0, imageSize);
                
        //         // clock_t start = clock();

              
        //         // bayer_device_src->copyFrom(tmpBuffer, bayer_device_src->pitch());
               
            
        //         // clock_t end = clock();
               
        //         // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(bayer_device_src->data(), (int)bayer_device_src->width(), {(int)bayer_device_src->width(), (int)bayer_device_src->height()}, 
        //         //             {0, 0, (int)bayer_device_src->width(),(int)bayer_device_src->height() }, (Npp8u *)rgb_device_dst.data(), (int)rgb_device_dst.width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100);
              
        //         // clock_t end = clock();

        //         //  free(buff_item.image);
        //         // buff_item.image = NULL;

        //         // double elapsed = (double(end-start))/CLOCKS_PER_SEC;
        //         // if (i % 20 ==0) 
        //         //     printf("time elapsed by write in second:%f\n",elapsed);

                
        //         // std::cout<<"status of bayer2rgb conversion:"<<stat<<std::endl;
        //         // std::this_thread::sleep_for(std::chrono::milliseconds(50));

        //         // SaveBayerAsTiff(filename, buff_item.image.get(), m_uWidth, m_uHeight);
        //         // CImagePersistence::Save(ImageFileFormat_Tiff, file_name, (void *)buff_item.image.get(), imageSize,  PixelType_BayerRG8, m_uWidth, m_uHeight, 0, ImageOrientation_TopDown );
        //         // start = clock();
        //         // std::ofstream outfile (filename);
        //         //     outfile.write(reinterpret_cast<char*>(tmpBuffer), imageSize);
        //         //     // outfile.write(reinterpret_cast<char*>( buff_item.image.get()), imageSize);
        //         //     // outfile.write(reinterpret_cast<char*>( buff_item.image.get()), imageSize);

                
        //         // outfile.close();

        //         // end = clock();
        //         // elapsed = double(end-start)/CLOCKS_PER_SEC;
        //         // printf("time elapsed by write in second:%f\n",elapsed);




        //         // bool res = converter->convertAndEncodeBayerToH264(buff_item.image.get(), nCurCameraIndex, buff_item.timeStamp);
        //         // if (res  )
        //         //         // converter->push
        //         //     converter->writeSingleFrame2MP4(nCurCameraIndex);
        //         // FPS_CALC ("Consuming from Buffer callback", nCurCameraIndex);

        //         i++;
        //         // if (i > 50) return 0;
               
        //  }
        //  return 1;
        //  m_bExit.store(true, std::memory_order_release);
         while (m_queueGrabRes[nCurCameraIndex].size() != 0 ){
                //  DATA buff_item = m_queueGrabRes[nCurCameraIndex].dequeue();

             DATA buff_item = m_queueGrabRes[nCurCameraIndex].front();
             m_queueGrabRes[nCurCameraIndex].pop();

            // DATA buff_item;
            // m_queueGrabRes[nCurCameraIndex].try_pop(buff_item);
            // while(!m_queueGrabRes[nCurCameraIndex].try_dequeue(buff_item));
            int imageSize = buff_item.imageSize;
            
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            std::string filename= "/mnt/m2_storage/test_images/GrabbedImage_" + std::to_string (buff_item.timeStamp/1000000) + "_" + std::to_string (nCurCameraIndex) + ".tiff";
            String_t file_name(filename.c_str());
            // delete [] buff_item.image;
            // buff_item.image.reset();
            
            // for (int j = 0 ; j < 15; j++)
            CImagePersistence::Save(ImageFileFormat_Tiff, file_name, (void *)buff_item.image, imageSize,  PixelType_BayerRG8, m_uWidth, m_uHeight, 0, ImageOrientation_TopDown );
            // SaveBayerAsTiff(filename, buff_item.image.get(), m_uWidth, m_uHeight);
            // CImagePersistence::Save(ImageFileFormat_Png, file_name, (void *)buff_item.image, imageSize,  PixelType_BayerRG8, m_uWidth, m_uHeight, 0, ImageOrientation_TopDown );

            
            free(buff_item.image);
            buff_item.image = NULL;
            // std::ofstream outfile (filename);
            // // outfile.write(reinterpret_cast<char*>(buff_item.image.get()), imageSize);
            // outfile.close();
          

            // std::cout<< nCurCameraIndex<<".Cam, timeStamp:"<<buff_item.timeStamp<<" "<< buff_item.serialNumber<< std::endl;
            
            if (m_queueGrabRes[nCurCameraIndex].size() % 10 == 0)
            {
                std::cout<<nCurCameraIndex<<". Cam, Buffer Size:"<< m_queueGrabRes[nCurCameraIndex].size()<<std::endl;
            }  
            i++;
         }
        
        
        
    //    delete [] tmpBuffer;
   

        
    
    return 0;
}

//Thread function with GetImageBuffer API
int BaslerMultipleCameras::ThreadMultiGrabFun(int nCurCameraIndex)
{

    // vec_data_struct[nCurCameraIndex].cam = &m_bsCameras[nCurCameraIndex];
    converter->InitializeSingleGstPipeline(nCurCameraIndex);
    // std::this_thread::sleep_until(m_tWakeupTime);
    // u_int8_t *tmpBuffer = (u_int8_t*)malloc(m_uHeight* m_uWidth);
    m_bsCameras[nCurCameraIndex].StartGrabbing( GrabStrategy_OneByOne, GrabLoop_ProvidedByUser);
    unsigned int i = 0;
    //   CBaslerUniversalGrabResultPtr ptrGrabResult;
    const int DefaultTimeout_ms = 2000000;
    // converter->CloseSingleGstPipeline(nCurCameraIndex);


    // return 1;
    // std::unique_lock<std::mutex> lock(m_mEndGrabMutexes_[nCurCameraIndex]);

    // m_cEndGrabConds_[nCurCameraIndex].wait(lock, [&](){

    //     return m_bExit.load(std::memory_order_acquire);
    // });
    // WaitObject::Sleep(20000);
//     m_waitObject.WaitEx(1000000,true);
    // vec_data_struct[nCurCameraIndex]->image = new uint8_t[m_uHeight* m_uWidth];


    CBaslerUniversalGrabResultPtr ptrGrabResult;
    while(/*!m_bExit.load(std::memory_order_acquire)*/ !m_bExit && m_bsCameras[nCurCameraIndex].IsGrabbing() )    {
        m_uTotalNumImgVec_[nCurCameraIndex]++;

        // std::cout<<"m_grab_flag:"<<m_bGrabExitFlag.load(std::memory_order_acquire)<<std::endl;
        // m_bsCameras[nCurCameraIndex].WaitForFrameTriggerReady(10000, TimeoutHandling_ThrowException);
        m_bsCameras[nCurCameraIndex].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
        intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
        if (ptrGrabResult->GrabSucceeded())
        {
           
            // if (i %10 == 0 /*&& (nCurCameraIndex ==2 || nCurCameraIndex ==1) */) 
            //     std::cout<<nCurCameraIndex<<". Cam, Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;
           
            uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            size_t bufferSize = ptrGrabResult->GetBufferSize();
            u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();
            const std::string serialNumber{m_bsCameras[nCurCameraIndex].GetDeviceInfo().GetSerialNumber().c_str()};
            // if (i > 250) {
            //     break; 
            //     m_bExit=true;
            // }

           {
        

            // std::lock_guard<std::mutex> lock(m_mProduceConsumeMutexes_[nCurCameraIndex]);
            // clock_t start = clock();
            // std::cout<<"buffer size in grab:"<<bufferSize<<std::endl;
          
           
            // uint8_t *tmpBuffer =  new uint8_t[bufferSize];// (u_int8_t*)malloc(width* height);
            // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpBuffer);
            
            // if (i > 100) 
            converter->push_data2(pImageBuffer, nCurCameraIndex);
            // vec_data_struct[nCurCameraIndex]->image = pImageBuffer;
            // vec_data_struct[nCurCameraIndex].image = new uint8_t[m_uHeight* m_uWidth];
            // vec_data_struct[nCurCameraIndex].imageSize = bufferSize;

            // std::copy(pImageBuffer, pImageBuffer + bufferSize, vec_data_struct[nCurCameraIndex].image);

            // converter->EncodeCuda(pImageBuffer, nCurCameraIndex);
            // memcpy(tmpBuffer, pImageBuffer, bufferSize);


            
            
            // DATA data{tmpBuffer, timeStamp, bufferSize, serialNumber};
            // m_queueGrabRes[nCurCameraIndex].push(data);

            

            }
            // m_cProduceConsumeConds_[nCurCameraIndex].notify_one();
            
            // DATA data{tmpSharedptr, timeStamp, bufferSize, serialNumber};
            // m_queueGrabRes[nCurCameraIndex].enqueue(data);
             // if (nCurCameraIndex == 0) 
            FPS_CALC("Grabbing Buffer FPS",  nCurCameraIndex);
        }
        else
        {
             m_uLossRatioVec_[nCurCameraIndex]++;
           
           std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << std::endl;//" " << ptrGrabResult->GetErrorDescription() << std::endl;
        }
        i++;


        


         ptrGrabResult.Release();

      

   
    } 
    // converter->StartSingleGstPipeline(nCurCameraIndex);

    converter->CloseSingleGstPipeline(nCurCameraIndex);
   

    // BayerToH264ConverterNvidiaCodec::exit_flag.store(true);
    // m_bExit=true;
    // ratio[nCurCameraIndex] = float(cnt)/i;
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
        // m_tlFactory.EnumerateDevices( m_allDeviceInfos);
    }
    catch (const GenericException& e)
    {
        PYLON_UNUSED( e );

        std::cerr<<e.GetDescription()<<std::endl;
    }
    m_uDeviceNum = 14;// m_allDeviceInfos.size() ;
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
           

        // For this sample we configure all cameras to be in the same group.
           
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
    m_uHeight = pt.get<unsigned int>("Height");
    m_uWidth = pt.get<unsigned int>("Width");
    m_fExposureTime = pt.get<float>("ExposureTime");
    m_fAcquisitionFrameRate = pt.get<float>("AcquisitionFrameRate");
    m_fGain = pt.get<float>("Gain");
    m_sPixelFormat = pt.get<std::string>("PixelFormat");
    m_uPacketSize =  pt.get<unsigned int>("PacketSize");
    m_uPacketDelay = pt.get<unsigned int>("PacketDelay");
    
    auto pixelFormatEnum = magic_enum::enum_cast<Basler_UniversalCameraParams::PixelFormatEnums>(m_sPixelFormat);
  

    
    file.close();
    // converter = std::make_unique<BayerToH264ConverterNvidiaCodec>(m_mapSerials, m_uDeviceNum, m_uWidth, m_uHeight, (unsigned int)m_fAcquisitionFrameRate);   
    converter = std::make_unique<BayerToH264ConverterGST>(m_mapSerials, m_uWidth, m_uHeight);   





    try
    {
        for (size_t i = 0; i < m_uDeviceNum; ++i)
        {
            
            // bayer_device_srcs_.push_back(std::make_unique< npp::ImageNPP_8u_C1>(m_uWidth, m_uHeight, true));
            // rgba_device_dsts_.push_back(std::make_unique< npp::ImageNPP_8u_C4>(m_uWidth, m_uHeight, true));

            // m_bsCameras[i].UserSetSelector.SetValue(Basler_UniversalCameraParams::UserSetSelectorEnums::UserSetSelector_Default);
            m_bsCameras[i].UserSetSelector.SetValue("Default");
            m_bsCameras[i].UserSetLoad.Execute();

            // std::cout<<"Max Grab Result Num:" <<m_bsCameras[i].MaxNumGrabResults.GetValue()<<std::endl;
            // std::cout<<"Max Buffer Num:" <<m_bsCameras[i].MaxNumBuffer.GetValue()<<std::endl;
            // std::cout<<"Max Queued Buffer Num:" <<m_bsCameras[i].MaxNumQueuedBuffer.GetValue()<<std::endl;
            // m_bsCameras[i].MaxNumBuffer.SetValue(100);
            // m_bsCameras[i].MaxNumQueuedBuffer.SetValue(100);

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

         
            
            // cout<<"ptp clock:"<<m_bsCameras[i].BslPeriodicSignalSource.GetValue()<<endl;
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

    // Calculate the time at which you want to wake up
    m_tWakeupTime = now + duration;
    // m_bsCameras.StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByInstantCamera);
    // m_tGrabThread = std::thread(std::bind(&BaslerMultipleCameras::ThreadSingleGrabFun, this));
    // m_tGrabThread = std::thread(std::bind(&BaslerMultipleCameras::ThreadSingleGrabFunWithActCommand, this));
    // std::this_thread::sleep_for(std::chrono::milliseconds(000));
    // m_tGrabThread.join();

    

    for (unsigned int i = 0; i < m_uDeviceNum; i++)
    {

        m_tGrabThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadMultiGrabFun, this, i)));
        std::cout<<i<<".Cam Grab Func just started!"<<std::endl;
              
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(20000));
    // WaitObject::Sleep(20000);

   
    for (auto &th: m_tGrabThreads)
    {
        if (th.joinable())
            th.join();
    
    }


    // for (unsigned int i = 0; i < m_uDeviceNum; i++)
    // {
      
    //     m_tConsumeThreads.emplace_back(std::thread(std::bind(&BaslerMultipleCameras::ThreadConsumeAnWrite2DiskAsMp4Fun, this, i)));
     
    // }

    
    return m_nExitCode;
    
    unsigned i = 0;
    float sum_elapsed = 0.0f;
    // std::vector<CBaslerUniversalGrabResultPtr> ptrGrabResults(m_uDeviceNum);
    // std::vector<std::vector<CBaslerUniversalGrabResultPtr>>  allPtrGrabResults;
    std::vector< std::future<void> > results;//(m_uDeviceNum);
    m_bsCameras.StartGrabbing(GrabStrategy_OneByOne,  GrabLoop_ProvidedByUser);
    while(!m_bGrabExitFlag && m_bsCameras.IsGrabbing() )
    {
        // This smart pointer will receive the grab result data.
        
      
        
        // if (m_bExit ) {
        //     m_bGrabExitFlag = true;
            
        // }
        // std::vector<CBaslerUniversalGrabResultPtr> ptrGrabResults(m_uDeviceNum);
        // allPtrGrabResults.push_back(ptrGrabResults);
        // CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;
        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {

            CBaslerUniversalGrabResultPtr ptrGrabResult;

            // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
            m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );

            // CBaslerUniversalGrabResultPtr &ptrGrabResult = ptrGrabResult;
            // if (ptrGrabResult->GrabSucceeded())
            // {
               
                
            //     u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();

            //     // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
            //     uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();

            //     size_t bufferSize = ptrGrabResult->GetBufferSize();
            //     int cameraIndex = ptrGrabResult->GetCameraContext();
            //     size_t cameraPadding = ptrGrabResult->GetPaddingX();
            //     const std::string serialNumber(m_bsCameras[j].GetDeviceInfo().GetSerialNumber().c_str());

            //     u_int8_t *tmpBuffer = (u_int8_t*)malloc(m_uHeight* m_uWidth);

            //     clock_t start = clock();
            //     // results.emplace_back(
            //     // m_threadPool.submit_task([this, &pImageBuffer, bufferSize, &start, j, timeStamp, serialNumber] {

            //     //         std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize/2]);
            //     //         // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpPtr);
            //     //         mem_cpy(tmpSharedptr.get(), pImageBuffer, bufferSize/2);
            //     //         clock_t end = clock();
            //     //         double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
            //     //         printf("time taken by memcpy in seconds : %f, %f, and %ld. Cam\n", elapsed, double(end)/CLOCKS_PER_SEC, j);
            //     //         DATA data{tmpSharedptr, timeStamp, bufferSize, serialNumber.c_str()};
            //     //         m_queueGrabRes[j].enqueue(data);

            //     // }));

            //     // std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
            //     // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpPtr);
            //     // memcpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
            //     // std::cout<<"buffer size in grab:"<<bufferSize<<std::endl;

            //     // mem_cpy(tmpBuffer, pImageBuffer, bufferSize);
            //     clock_t end = clock();
            //     double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
            //     // printf("time taken by memcpy in seconds : %f, %f, and %ld. Cam\n", elapsed, double(end)/CLOCKS_PER_SEC, j);
            //     if (j == 0 ) sum_elapsed += elapsed;
            //     if (j == 0  && i %20 == 0)
            //         std::cout<<"Average elaped time with memcpy in seconds :"<< sum_elapsed/i<<" "<<bufferSize<<std::endl;
                
            //     // {
            //     //    std::lock_guard<std::mutex> lock(m_mProduceConsumeMutexes_[j]);
            //        DATA data{tmpBuffer, timeStamp, bufferSize, serialNumber};
            //        m_queueGrabRes[j].enqueue(data);
            // //     }
                
               
            // //    m_cProduceConsumeConds_[j].notify_one();


            // }
            // else
            // {
            //     // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
            //     // multiple images simultaneously. See note above c_maxCamerasToUse.
            //     std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
            // }
            // // ptrGrabResult.Release();
        
            // if (j == 0) FPS_CALC_BUF2("Grabbing Buffer FPS");


        }
        // for(int k =0 ; k < results.size(); k++)
        //         results[k].wait();
        // results.clear();
        i++;
      
  

    }


   
   return m_nExitCode;

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
        // This smart pointer will receive the grab result data.
        
      
        
        // if (m_bExit ) {
        //     m_bGrabExitFlag = true;
            
        // }
        // std::vector<CBaslerUniversalGrabResultPtr> ptrGrabResults(m_uDeviceNum);
        // allPtrGrabResults.push_back(ptrGrabResults);
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;
        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {

            // CBaslerUniversalGrabResultPtr ptrGrabResult;

            // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
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

                // clock_t start = clock();
                // results.emplace_back(
                // m_threadPool.submit_task([this, &pImageBuffer, bufferSize, &start, j, timeStamp, serialNumber] {

                //         std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize/2]);
                //         // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpPtr);
                //         mem_cpy(tmpSharedptr.get(), pImageBuffer, bufferSize/2);
                //         clock_t end = clock();
                //         double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
                //         printf("time taken by memcpy in seconds : %f, %f, and %ld. Cam\n", elapsed, double(end)/CLOCKS_PER_SEC, j);
                //         DATA data{tmpSharedptr, timeStamp, bufferSize, serialNumber.c_str()};
                //         m_queueGrabRes[j].enqueue(data);

                // }));

                // std::shared_ptr<uint8_t[]>  tmpSharedptr (new uint8_t[bufferSize]);
                // std::copy(pImageBuffer, pImageBuffer + bufferSize, tmpPtr);
                // memcpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
                // std::cout<<"buffer size in grab:"<<bufferSize<<std::endl;

                memcpy(tmpBuffer, pImageBuffer, bufferSize);
                // clock_t end = clock();
                // double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
                // printf("time taken by memcpy in seconds : %f, %f, and %ld. Cam\n", elapsed, double(end)/CLOCKS_PER_SEC, j);
                // if (j == 0 ) sum_elapsed += elapsed;
                // if (j == 0  && i %20 == 0)
                //     std::cout<<"Average elaped time with memcpy in seconds :"<< sum_elapsed/i<<" "<<bufferSize<<std::endl;
                
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
            // ptrGrabResult.Release();
        
            // if (j == 0) FPS_CALC_BUF2("Grabbing Buffer FPS");


        }
        // for(int k =0 ; k < results.size(); k++)
        //         results[k].wait();
        // results.clear();
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
        // m_bsCameras[0].WaitForFrameTriggerReady(10000, TimeoutHandling_ThrowException);
        m_pTL->IssueScheduledActionCommand(m_iDeviceKey, m_iGroupKey, m_iAllGroupMask, actionTime);
        // m_pTL->IssueActionCommand(m_iDeviceKey, m_iGroupKey, m_iAllGroupMask);
        CBaslerUniversalGrabResultPtr ptrGrabResult;
        const int DefaultTimeout_ms = 5000;

        for (size_t j = 0; j < m_uDeviceNum ;++j)
        {
            // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
            m_bsCameras[j].RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
            // if (ptrGrabResult->GrabSucceeded()) std::cout<<j<<".cam's result grab succeded"<<std::endl;
            // std::cout<<j<<".cam PTP status:"<<m_bsCameras[j].PtpStatus.GetValue()<< " "<<m_bsCameras[j].PtpServoStatus.GetValue()<<std::endl;


            // if (ptrGrabResult->GrabSucceeded()) 
            // {
            //     u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();
            //     // std::cout<<j<<".cam's result grab succeded "<<timeStamp<<std::endl; 
              

            //     // std::cout << ptrGrabResult->GetCameraContext()<<" .Cam TimeStamp:" << timeStamp << std::endl;
            //     uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
            //     size_t bufferSize = ptrGrabResult->GetBufferSize();
            //     int cameraIndex = ptrGrabResult->GetCameraContext();
            //     size_t cameraPadding = ptrGrabResult->GetPaddingX();
            //     const char *serialNumber = {m_bsCameras[j].GetDeviceInfo().GetSerialNumber().c_str()};
            //     // std::cout<<"cameraPadding:"<<cameraPadding<< " "<<ptrGrabResult->GetWidth()<<std::endl;
            

            //     //std::shared_ptr<uint8_t[]>  tmpSharedptr(pImageBuffer);
            //     // std::unique_ptr<uint8_t[]>  tmpUniqueptr (pImageBuffer);
            //     std::shared_ptr<uint8_t>  tmpSharedptr (new uint8_t[bufferSize]);
            //     mem_cpy(tmpSharedptr.get(), pImageBuffer, bufferSize);
            //     // DATA data{tmpSharedptr, timeStamp, bufferSize, {serialNumber}};
            //     // m_queueGrabRes[j].enqueue(data);
            // }
            // else
            // {
            //     // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
            //     // multiple images simultaneously. See note above c_maxCamerasToUse.
            //     std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << std::endl;
            // }
            // ptrGrabResult.Release();


        }

     //   FPS_CALC_THREAD_BUF("Grabbing Buffer FPS",  m_queueGrabRes, 0);
        


    
    }
    return 0;
}




// Stop grabbing
int BaslerMultipleCameras::StopGrabbing()
{
  
   

  

    // std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // m_tGrabThread.join();


    // for (auto &th: m_tGrabThreads)
    // {
    //     if (th.joinable())
    //         th.join();
    
    // }

    
    // for (auto &th: m_tConsumeThreads)
    // {
    //     if (th.joinable())
    //       th.join();
        
    // }
   
    
   

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


// Software trigger

// GigE Action Command Trigger



