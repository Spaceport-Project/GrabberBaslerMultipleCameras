// header file

#pragma once
#include <stdio.h>
#include <thread>
#include <condition_variable>
#include <memory>
#include <cstdint>
#include <map>
#include <queue>
#include <portaudio.h>

// #include <cstdlib>
#include <immintrin.h>
// #include <cstdint>
#include <stdlib.h>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/thread_pool.hpp>
#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_priority_queue.h>

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
// #include "Bayer2H264ConverterFFMPEG.h"
#include "Bayer2H264ConverterNvidiaCodec.h"
// #include "Bayer2H264ConverterGST.h"
// #include "SharedQueue.h"
#include "ImageBuffer.h"
// #include "SafeVector.h"
#include "SafeQueue.h"
// #include "concurrentqueue.h"
#include "BS_thread_pool.hpp" // BS::thread_pool
#include "ThreadPool.h"

#include <ImagesCPU.h>
#include <ImagesNPP.h>

#include "NvEncoderCuda.h"

// #include "memcopy_func.h"


using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;


#define FPS_CALC_BUF2(_WHAT_) \
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
      std::cerr << "Average framerate("<< _WHAT_ << "): " << double(count_buf)/double(now_buf - last_buf) << " Hz. Queue size: " << " Frame Number: "<<counter <<"\n"; \
      std::cerr << "\033[0m";\
      count_buf = 0; \
      last_buf = now_buf; \
    } \
}while(false)


struct DATA{
 
    // std::shared_ptr<uint8_t[]>  image;
    u_int8_t *image;
    // std::vector<u_int8_t> image;
    u_int64_t timeStamp=0;
    size_t imageSize=0;
    std::string serialNumber;
};



class BaslerMultipleCameras 
{
// Construction
public:
    
    using threadVector = std::vector<std::thread>;
    using timePoint = std::chrono::system_clock::time_point;
    using byteArrayVector = std::vector<std::shared_ptr<uint8_t[]>> ;
    using condVector = std::vector<std::condition_variable>;
    // static std::atomic<bool> m_bExit;
    static bool m_bExit;


    static WaitObjectEx m_waitObject;


	BaslerMultipleCameras(const std::string&);
    ~BaslerMultipleCameras();	      
   
private:
    unsigned int            m_uDeviceNum = 0;
    const std::string&      m_sCameraSettingsFile;
   
   
    threadVector            m_tGrabThreads;
    std::thread                  m_tGrabThread;
    threadVector            m_tConsumeThreads;
   
    std::thread                  m_tCheckBuffThread;
    threadVector            m_tOpenDevicesThreads;
    threadVector            m_tWriteMP4Threads;
    condVector              m_cProduceConsumeConds_;

    std::unique_ptr<BayerToH264ConverterNvidiaCodec> converter;
    // std::unique_ptr<BayerToH264ConverterGST> converter;


    std::mutex              m_mWriteMp4Mutex;
    std::vector<std::mutex> m_mProduceConsumeMutexes_;
  
    std::map<int, std::string> m_mapSerials; 
    std::map<int, std::string> m_mapModels; 
    // std::vector<ImageBuffer<CBaslerUniversalGrabResultPtr> > m_queueGrabRes;
    // std::vector<SafeQueue<DATA>>  m_queueGrabRes;
    std::vector<std::queue<DATA>>  m_queueGrabRes;
    std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> bayer_device_srcs_;
    std::vector<CUcontext> cu_contexts_;
    std::vector<CUdevice> cuDevices_;


    // std::vector<moodycamel::ConcurrentQueue<DATA>> m_queueGrabRes;
    // std::vector<boost::lockfree::queue<DATA, boost::lockfree::capacity<500> >> m_queueGrabRes;
    // std::vector<tbb::concurrent_queue<DATA>> m_queueGrabRes;

    // ThreadPool m_threadPool;
    // std::ofstream outfile;

    // std::shared_ptr<Sa<DATA>[]> m_queueGrabRes;

    int m_nExitCode = 0;
    CBaslerUniversalInstantCameraArray m_bsCameras;
    IGigETransportLayer* m_pTL = nullptr;
    CTlFactory &m_tlFactory;
    // CTlFactory m_tlFactoryInst;
    DeviceInfoList_t m_allDeviceInfos;
    std::chrono::system_clock::time_point m_tWakeupTime;
    uint32_t m_iDeviceKey;
    uint32_t m_iAllGroupMask;
    uint32_t m_iGroupKey;
    unsigned int m_uHeight ;
    unsigned int m_uFrameNum;
    unsigned int m_uWidth ;
    unsigned int m_uPacketSize;
    unsigned int m_uPacketDelay;
    float m_fExposureTime ;
    float m_fAcquisitionFrameRate;
    float m_fGain;
    // std::atomic<bool> m_bGrabExitFlag{false};
    bool m_bGrabExitFlag =false;
    
    std::string m_sPixelFormat;
    // std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> bayer_device_srcs_;
    // std::vector<std::unique_ptr< npp::ImageNPP_8u_C4>> rgba_device_dsts_;
    // std::vector<cudaStream_t> cuda_streams_;
    // std::vector<NppStreamContext> npp_stream_contextes_;
    std::vector<unsigned int> m_uLossRatioVec_;
    std::vector<unsigned int> m_uTotalNumImgVec_;

    // std::vector<std::unique_ptr<NvEncoderCuda>> pEncs_;

    std::vector<bool> m_bStarters_;
    std::atomic<bool> m_bStarter_ = false;
    u_int64_t m_initTimeStamp_;
    std::chrono::system_clock::time_point m_timePoint_;


    struct AudioSample {
        float leftSample;
        float rightSample;
        std::chrono::system_clock::time_point timestamp;    
    };
    struct AudioData {
        std::vector<AudioSample> recordedSamples;
        bool isRecording;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point initialTimestamp;
        uint64_t sampleCount;  // Add this line
    };
    const static unsigned int  SAMPLE_RATE = 44100;
    const static unsigned long  FRAMES_PER_BUFFER = 256;
    const static  int  NUM_CHANNELS = 2;

    // Structure to hold WAV file header
#pragma pack(push, 1)
    struct WAVHeader {
        // RIFF chunk
        char riffId[4] = {'R', 'I', 'F', 'F'};
        uint32_t riffSize;
        char waveId[4] = {'W', 'A', 'V', 'E'};
        
        // fmt chunk
        char fmtId[4] = {'f', 'm', 't', ' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 3; // IEEE float
        uint16_t numChannels = NUM_CHANNELS;
        uint32_t sampleRate = SAMPLE_RATE;
        uint32_t byteRate = SAMPLE_RATE * NUM_CHANNELS * sizeof(float);
        uint16_t blockAlign = NUM_CHANNELS * sizeof(float);
        uint16_t bitsPerSample = sizeof(float) * 8;
    };

    struct TimeChunkHeader {
        char timeId[4] = {'T', 'I', 'M', 'E'};
        uint32_t timeSize;
    };

    struct TimestampData {
        uint64_t sampleIndex;
        uint64_t timestamp; // microseconds since epoch
    };
#pragma pack(pop)
    
    
    AudioData m_soundData_;
    std::mutex m_soundMutex_;
    std::condition_variable m_soundCond_;
    std::thread m_soundThread_;


  
   
  
 
public:
    static bool SaveBayerAsTiff(const std::string &file_name, uint8_t *buffer, uint32_t width, u_int32_t height);
    static int recordCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData);
    void EnumDevices();

    int OpenDevices();
    int StartGrabbing();
    int ThreadSingleGrabFun();
    int ThreadSingleGrabFunWithActCommand();
    int CloseDevices();
    int StopGrabbing();
    int ConfigureCameraSettings();

    int  Save2BufferThenDisk();
    int OpenDevicesInThreads();
    void CloseDevicesInThreads();
    
    

    int ThreadConsumeAnWrite2DiskAsMp4Fun(int );
    // int ThreadMultiGrabFun(int nCurCameraIndex, std::vector<float>  &ratio);
    int ThreadMultiGrabFun(int nCurCameraIndex);
    int ThreadOpenDevicesFun(int);
    int ThreadCloseDevicesFun(int );
    // std::vector<ImageBuffer<std::unique_ptr<uint8_t[]> >> & GetQueueVectors() {return m_queueGrabRes;};
    friend class  CBaslerImageEventHandler;

    int StartSoundRecording();
    int ThreadStartSoundRecording();
    void saveToWavWithEmbeddedTimestamps(const std::vector<AudioSample> &samples, const char *audioFile);

private:
   
    void Write2H264FromBayer(int numWriteThreads);




};


