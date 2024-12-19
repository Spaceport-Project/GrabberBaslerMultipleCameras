

#ifndef BAYER2H264_NVIDIACODEC_H
#define BAYER2H264_NVIDIACODEC_H

#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <atomic>
#include <map>
// #include <cuda.h>
// #include <cuda_runtime.h>
#include <mutex>

// #include <NvEncodeAPI.h>


#include <cuda.h>
#include "NvCodecUtils.h"
#include "NvEncoderCuda.h"
#include "NvEncoderCLIOptions.h"
#include "NvCodecUtils.h"
#include "NvEncoderOutputInVidMemCuda.h"
#include "Logger.h"

// #include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>
#include <opencv2/highgui.hpp>


class NvCUStream;
class BayerToH264ConverterNvidiaCodec{
public:
    using NvEncPtr = std::unique_ptr<NvEncoder, std::function<void(NvEncoder*)>>;

    BayerToH264ConverterNvidiaCodec(const std::vector<CUcontext> &cu_contexts, std::map<int, std::string> map_serial_nums, unsigned int device_num, unsigned int input_width, unsigned int input_height, unsigned int fps, float resize_factor, const  std::chrono::system_clock::time_point &);
  
    bool close() ;

    ~BayerToH264ConverterNvidiaCodec() ;
   

     void InitializeEncoder( NvEncPtr &pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat);
    void InitializeEncoder( std::unique_ptr<NvEncoderOutputInVidMemCuda> &pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat);
    void  EncodeCudaFromDevice(const std::unique_ptr< npp::ImageNPP_8u_C1>   &bayerDevice, int n_cam_index,  uint64_t timestamp,  bool = false, unsigned int cnt = 0xFFFFFFFF);
    void  EncodeCudaFromDevice( int n_cam_index, bool flag_exit);

    void EncodeCuda(uint8_t * &pHostFrame, int n_cam_index);
    void EncodeCudaOpInVidMem(uint8_t * &pHostFrame, int n_cam_index);
   
    bool convertAndEncodeBayerToH264( uint8_t *bayerData, unsigned int n_curr_cam_index,  int64_t time_stamp) ;
    void CopyImageFromHost2Device( uint8_t *, int);
    static void ShowEncoderCapability();
    // std::vector<CUcontext> & getCuContexts() {return cu_contexts_;};
   
    std::vector<bool> & getResults();

    bool writeSingleFrame2MP4(int nCurrCameraIndex);

    
    static std::atomic<bool> exit_flag;

    private:
        void initialize();
           
       

    
    private:
        const unsigned int width_;
        const unsigned int height_;
        const unsigned int cnt_limit = 30;


        unsigned int num_devices_;
        unsigned int frameIndex = 0;
        unsigned int fps_;
        std::vector<bool> results_;
        std::vector<std::mutex> codecMutexes_;
        std::vector< unsigned int>  frame_cnts;
        std::ofstream outputFile;
        // std::vector<std::unique_ptr< npp::ImageNPP_8u_C1>> bayer_device_srcs_;
        std::vector<std::unique_ptr<npp::ImageNPP_8u_C4>> rgba_device_dsts_;
        std::vector<std::unique_ptr<npp::ImageNPP_8u_C4>> rgba_device_dsts_resized_;
        NppiSize image_size_ ;
        NppiRect image_roi_ ;
        NppiSize image_size_resized_ ;
        NppiRect image_roi_resized_ ;
        float resize_factor_;


        NvEncoderInitParam encode_CLI_options_;
        NV_ENC_BUFFER_FORMAT enc_format_ ;//= NV_ENC_BUFFER_FORMAT_ARGB;
        const std::vector<CUcontext> &cu_contexts_;
        // CUcontext cu_context_ = nullptr ;
        std::vector<cudaStream_t> cuda_streams_;
        std::vector<NppStreamContext> npp_stream_contextes_;


        std::vector<NvEncPtr> pEncsCuda_;
        std::vector<NvEncPtr> pEncsCudaOrg_;

        std::vector<std::unique_ptr<NvEncoderOutputInVidMemCuda>>  pEncsVidCuda_;
        // std::vector<std::unique_ptr<NvCUStream>> p_cu_streams_;
        std::vector<CUdeviceptr> bayer_dp_buf_vec_;
        std::vector<CUdeviceptr> rgba_dp_buf_vec_;

        std::vector<CUdevice> cuDevices_;
        std::vector<std::ofstream> fp_outs_;
        std::vector<std::ofstream> fp_outs_org_;

        std::map<int, std::string> &map_serial_nums_;
        const std::chrono::system_clock::time_point &time_point_;
        // std::vector<cudaStream_t> cuda_streams_;
        // std::vector<NppStreamContext> npp_stream_contextes_;
        


};

#endif