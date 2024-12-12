#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
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
#include <opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>



#include <cuda.h>
#include "NvCodecUtils.h"
#include "NvEncoderCuda.h"
#include "NvEncoderCLIOptions.h"
#include "NvEncoderOutputInVidMemCuda.h"

// #include <ImageIO.h>
auto EncodeDeleteFunc = [](NvEncoder *pEnc)
{
    if (pEnc)
    {
        pEnc->DestroyEncoder();
        delete pEnc;
    }
};
class NvCUStream
{
public:
	NvCUStream(CUcontext cuDevice, int cuStreamType, std::unique_ptr<NvEncoderOutputInVidMemCuda> &pEnc)
	{
		device = cuDevice;
		CUDA_DRVAPI_CALL(cuCtxPushCurrent(device));

		// Create CUDA streams
		if (cuStreamType == 1)
		{
			ck(cuStreamCreate(&inputStream, CU_STREAM_DEFAULT));
			outputStream = inputStream;
		}
		else if (cuStreamType == 2)
		{
			ck(cuStreamCreate(&inputStream, CU_STREAM_DEFAULT));
			ck(cuStreamCreate(&outputStream, CU_STREAM_DEFAULT));
		}

		CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));

		// Set input and output CUDA streams in driver
		pEnc->SetIOCudaStreams((NV_ENC_CUSTREAM_PTR)&inputStream, (NV_ENC_CUSTREAM_PTR)&outputStream);
	}

	~NvCUStream()
	{
		ck(cuCtxPushCurrent(device));

		if (inputStream == outputStream)
		{
			if (inputStream != NULL)
				ck(cuStreamDestroy(inputStream));
		}
		else
		{
			if (inputStream != NULL)
				ck(cuStreamDestroy(inputStream));

			if (outputStream != NULL)
				ck(cuStreamDestroy(outputStream));
		}

		ck(cuCtxPopCurrent(NULL));
	}

	CUstream GetOutputCUStream() { return outputStream; };
	CUstream GetInputCUStream() { return inputStream; };

private:
	CUcontext device;
	CUstream inputStream = NULL, outputStream = NULL;
};


#include "Bayer2H264ConverterNvidiaCodec.h"
#define ENSURE(expr) do { if (expr) break; std::printf("Error: %s\n", #expr); std::abort(); } while (false)



     
    BayerToH264ConverterNvidiaCodec::BayerToH264ConverterNvidiaCodec( const std::vector<CUcontext> &cu_contexts, std::map<int, std::string> map_serial_nums, unsigned int device_num, unsigned int input_width, unsigned int input_height, unsigned int fps, float resize_factor, const std::chrono::system_clock::time_point & time_point):
        cu_contexts_(cu_contexts),
        width_(input_width), 
        height_(input_height),
        resize_factor_(resize_factor),
        num_devices_(device_num),
        fps_(fps),
        map_serial_nums_(map_serial_nums),
        time_point_(time_point)
        
    {
        enc_format_ = NV_ENC_BUFFER_FORMAT_ARGB;
  
        initialize();

    }
  
    bool BayerToH264ConverterNvidiaCodec::close() {

        
        for (auto & pEnc: pEncsVidCuda_)
            pEnc->DestroyEncoder();
        for (auto & pEnc: pEncsCuda_)
            pEnc->DestroyEncoder();

        return true;
    }

    BayerToH264ConverterNvidiaCodec::~BayerToH264ConverterNvidiaCodec() 
    {
       
       close();
     
        for (int i =0; i < cu_contexts_.size() ;i++)
         ck(cuCtxDestroy(cu_contexts_[i]));

    }

   

    
     void  BayerToH264ConverterNvidiaCodec::InitializeEncoder( NvEncPtr  &pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat)
    {
        NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
        NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

        initializeParams.encodeConfig = &encodeConfig;
        pEnc->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
        encodeCLIOptions.SetInitParams(&initializeParams, eFormat);
        pEnc->CreateEncoder(&initializeParams);
     
    }

    void  BayerToH264ConverterNvidiaCodec::InitializeEncoder( std::unique_ptr<NvEncoderOutputInVidMemCuda> &pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat)
    {
        NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
        NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

        initializeParams.encodeConfig = &encodeConfig;
        pEnc->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
        encodeCLIOptions.SetInitParams(&initializeParams, eFormat);

        pEnc->CreateEncoder(&initializeParams);
    }
    
    void  BayerToH264ConverterNvidiaCodec::EncodeCudaFromDevice(const std::unique_ptr< npp::ImageNPP_8u_C1>  & bayer_device_src, int n_cam_index,  uint64_t timestamp, bool flag_exit)
    {
        NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(bayer_device_src->data(), (int)bayer_device_src->width(), {(int)bayer_device_src->width(), (int)bayer_device_src->height()}, 
                            {0, 0, (int)bayer_device_src->width(),(int)bayer_device_src->height() }, (Npp8u *)rgba_device_dsts_[n_cam_index]->data(), (int)rgba_device_dsts_[n_cam_index]->width()*4, NPPI_BAYER_BGGR, NPPI_INTER_UNDEFINED, 100);
        
        if (resize_factor_ != 1.0f)
            NppStatus result = nppiResize_8u_C4R(rgba_device_dsts_[n_cam_index]->data(), rgba_device_dsts_[n_cam_index]->pitch(), image_size_, image_roi_, 
                rgba_device_dsts_resized_[n_cam_index]->data(), rgba_device_dsts_resized_[n_cam_index]->pitch(), image_size_resized_, image_roi_resized_, NPPI_INTER_LANCZOS);
        
        std::vector<std::vector<uint8_t>> vPacket;
         

        if (!flag_exit)
        {
            const NvEncInputFrame* encoderInputFrame = pEncsCuda_[n_cam_index]->GetNextInputFrame();
            
            clock_t start = clock();
             if (resize_factor_ == 1.0f ) 
            NvEncoderCuda::CopyToDeviceFrame((CUcontext)pEncsCuda_[n_cam_index]->GetDevice(), rgba_device_dsts_[n_cam_index]->data(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                (int)encoderInputFrame->pitch,
                pEncsCuda_[n_cam_index]->GetEncodeWidth(),
                pEncsCuda_[n_cam_index]->GetEncodeHeight(),
                CU_MEMORYTYPE_DEVICE,
                encoderInputFrame->bufferFormat,
                encoderInputFrame->chromaOffsets,
                encoderInputFrame->numChromaPlanes,
                false
                );
            else 
                NvEncoderCuda::CopyToDeviceFrame((CUcontext)pEncsCuda_[n_cam_index]->GetDevice(), rgba_device_dsts_resized_[n_cam_index]->data(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
                (int)encoderInputFrame->pitch,
                pEncsCuda_[n_cam_index]->GetEncodeWidth(),
                pEncsCuda_[n_cam_index]->GetEncodeHeight(),
                CU_MEMORYTYPE_DEVICE,
                encoderInputFrame->bufferFormat,
                encoderInputFrame->chromaOffsets,
                encoderInputFrame->numChromaPlanes,
                false
                );



            
            pEncsCuda_[n_cam_index]->EncodeFrame(vPacket, timestamp);
            //  clock_t end = clock();
            // std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            // std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << n_cam_index <<". Cam"<<std::endl;


        }
        else
        {
            pEncsCuda_[n_cam_index]->EndEncode(vPacket);
        }
        for (std::vector<uint8_t> &packet : vPacket)
        {
            // For each encoded packet
            fp_outs_[n_cam_index].write(reinterpret_cast<char*>(packet.data()), packet.size());
        }
       
        if (flag_exit) return ;


    }
    



   
    bool  BayerToH264ConverterNvidiaCodec::convertAndEncodeBayerToH264( uint8_t *bayerData, unsigned int n_curr_cam_index,  int64_t time_stamp) 
    {
       
        
        
        clock_t start = clock();
      

       


         clock_t stop = clock();

        double elapsed = ((double)(stop - start)) / CLOCKS_PER_SEC;

       
        return true;

      
    }
  
   
    std::vector<bool> &  BayerToH264ConverterNvidiaCodec::getResults()
    {
        return results_;
    }
    

    bool  BayerToH264ConverterNvidiaCodec::writeSingleFrame2MP4(int nCurrCameraIndex) {

        
        return true;

    }
    

    void BayerToH264ConverterNvidiaCodec::ShowEncoderCapability()
    {
        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        std::cout << "Encoder Capability" << std::endl << std::endl;
        for (int iGpu = 0; iGpu < nGpu; iGpu++) {
            CUdevice cuDevice = 0;
            ck(cuDeviceGet(&cuDevice, iGpu));
            char szDeviceName[80];
            ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
            CUcontext cuContext = NULL;
            ck(cuCtxCreate(&cuContext, 0, cuDevice));
            NvEncoderCuda enc(cuContext, 1280, 720, NV_ENC_BUFFER_FORMAT_NV12);

            std::cout << "GPU " << iGpu << " - " << szDeviceName << std::endl << std::endl;
            std::cout << "\tH264:\t\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
                NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
                "\tH264_444:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
                NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
                "\tH264_ME:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
                NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl <<
                "\tH264_WxH:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
                NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl <<
                "\tHEVC:\t\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
                "\tHEVC_Main10:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORT_10BIT_ENCODE) ? "yes" : "no") << std::endl <<
                "\tHEVC_Lossless:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE) ? "yes" : "no") << std::endl <<
                "\tHEVC_SAO:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORT_SAO) ? "yes" : "no") << std::endl <<
                "\tHEVC_444:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
                "\tHEVC_ME:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl <<
                "\tHEVC_WxH:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
                NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl <<
                "\tAV1:\t\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
                NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
                "\tAV1_444:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
                NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
                "\tAV1_WxH:\t" << "  " <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
                NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
                (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl;

            std::cout << std::endl;

            enc.DestroyEncoder();
            ck(cuCtxDestroy(cuContext));
        }
    }

    
    void  BayerToH264ConverterNvidiaCodec::initialize(){
        
        ShowEncoderCapability();
        std::ostringstream oss;
        oss<<"-fps "<<fps_<<" -cq 31";

        encode_CLI_options_ = NvEncoderInitParam(oss.str().c_str());

      
        unsigned int sep_cam_num =  std::ceil(num_devices_*19.0/24);
        
        auto now = time_point_;// std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
           
        ss << std::put_time(std::localtime(&now_t), "%Y-%m-%d_%H-%M-%S");
        std::string folderName = "../recordings/" + ss.str();
        if (mkdir(folderName.c_str(), 0777) == 0 || errno == EEXIST) {
            std::cout << folderName <<" directory created or already exists." << std::endl;
        } else {
            std::cerr << "Failed to create "<< folderName<<" directory." << std::endl;
            return ;
        }



        for (unsigned int i = 0 ; i < num_devices_; i++)
        {
            
        

            cuCtxPushCurrent(cu_contexts_[i/sep_cam_num]);
            rgba_device_dsts_.push_back(std::make_unique<npp::ImageNPP_8u_C4> (width_, height_, true));
            rgba_device_dsts_resized_.push_back(std::make_unique<npp::ImageNPP_8u_C4> (width_/resize_factor_, height_/resize_factor_, true));


            // rgba_dp_buf_vec_.push_back((CUdeviceptr)rgba_device_dsts_.back()->data());
            
            

            std::string file_name = "/Cam_" + map_serial_nums_[i]  + ".mp4" ;
            std::string file_path = folderName + file_name;
            std::cout<<"Saving to "<<file_path<<std::endl;
            fp_outs_.push_back(std::ofstream(file_path, std::ios::out | std::ios::binary));
            
            NvEncPtr pEnc(new NvEncoderCuda(cu_contexts_[i/sep_cam_num], width_/resize_factor_, height_/resize_factor_, enc_format_), EncodeDeleteFunc);
        
            // pEncsVidCuda_.push_back(std::make_unique< NvEncoderOutputInVidMemCuda>(cu_contexts_[i], width_, height_, enc_format_));

            InitializeEncoder(pEnc, encode_CLI_options_, enc_format_);
            // p_cu_streams_.push_back(std::make_unique<NvCUStream>(reinterpret_cast<CUcontext>(pEncsVidCuda_.back()->GetDevice()), 1, pEncsVidCuda_.back()));
            
            pEncsCuda_.push_back(std::move(pEnc));
            cuCtxPopCurrent(nullptr);

        
        }

        image_size_ = {.width= (int)width_, .height = (int)height_};
        image_roi_ = {.x = 0, .y = 0, .width= (int)width_, .height = (int)height_};

        image_size_resized_ = {.width= (int)(width_/resize_factor_), .height = (int)(height_/resize_factor_)};
        image_roi_resized_= {.x = 0, .y = 0, .width= (int)(width_/resize_factor_), .height = (int)(height_/resize_factor_)};

        
         
        

    }
    

  