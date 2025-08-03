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
#include "Barrier.h"
// #include "memcopy_func.h"


using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;



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
    
    
    static bool m_bExit;




	BaslerMultipleCameras(const std::string&);
    ~BaslerMultipleCameras();	      
   
private:
    unsigned int            m_uDeviceNum = 0;
    const std::string&      m_sCameraSettingsFile;
    int                     m_uFullResCntLimit;
    const float             m_frameLossRation = 0.01;
    // Barrier                 m_Barrier_;
   
    threadVector            m_tGrabThreads;
    std::thread             m_tGrabThread;
    threadVector            m_tConsumeThreads;
    threadVector            configure_cam_threads;

   
    std::thread             m_tCheckBuffThread;
    threadVector            m_tOpenDevicesThreads;
    threadVector            m_tWriteMP4Threads;
    condVector              m_cProduceConsumeConds_;

    std::unique_ptr<BayerToH264ConverterNvidiaCodec> converter;


    std::mutex              m_mWriteMp4Mutex;
    std::mutex   m_fpsMutex;
    std::vector<std::mutex> m_mProduceConsumeMutexes_;
  
    std::map<int, std::string> m_mapSerials; 
    std::map<int, std::string> m_mapModels; 
 
    std::vector<std::queue<DATA>>  m_queueGrabRes;
    std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> bayer_device_srcs_;
    

    std::vector<CUcontext> cu_contexts_;
    std::vector<CUdevice> cuDevices_;


    int m_nExitCode = 0;
    CBaslerUniversalInstantCameraArray m_bsCameras;
    IGigETransportLayer* m_pTL = nullptr;
    CTlFactory &m_tlFactory;
    DeviceInfoList_t m_allDeviceInfos;
    std::chrono::system_clock::time_point m_tWakeupTime;
    uint32_t m_iDeviceKey;
    uint32_t m_iAllGroupMask;
    uint32_t m_iGroupKey;
    unsigned int m_uNumCams;
    unsigned int m_uHeight ;
    unsigned int m_uFrameNum;
    unsigned int m_uWidth ;
    float m_fResizeFactor_;
    unsigned int m_uPacketSize;
    unsigned int m_uTransDelay, m_uInterPacketDelay;
    float m_fExposureTime ;
    float m_fAcquisitionFrameRate;
    float m_fGain;
    bool m_bGrabExitFlag =false;
    
    std::string m_sPixelFormat;
    Basler_UniversalCameraParams::PixelFormatEnums m_pixelFormat;
 
    std::vector<unsigned int> m_uLossRatioVec_;
    std::vector<unsigned int> m_uTotalNumImgVec_;


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

    int ConfigureCameraSettingsinThreads();

    int Save2BufferThenDisk();
    int OpenDevicesInThreads();
    
    

    int ThreadConsumeAnWrite2DiskAsMp4Fun(int );
    int ThreadMultiGrabFun(int nCurCameraIndex);
    int ThreadOpenDevicesFun(int);
    friend class CBaslerImageEventHandler;

    int StartSoundRecording();
    int ThreadStartSoundRecording();
    void saveToWavWithEmbeddedTimestamps(const std::vector<AudioSample> &samples, const char *audioFile);

private:
   
    bool ReadCameraSettingsJson();

    void ThreadConfigureCamSettings(int camId);
};


