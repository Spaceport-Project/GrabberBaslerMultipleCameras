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



     
    BayerToH264ConverterNvidiaCodec::BayerToH264ConverterNvidiaCodec(std::map<int, std::string> map_serial_nums, unsigned int device_num, unsigned int input_width, unsigned int input_height, unsigned int fps):
        width_(input_width), 
        height_(input_height),
        num_devices_(device_num),
        fps_(fps),
        map_serial_nums_(map_serial_nums)
        
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
    
    void  BayerToH264ConverterNvidiaCodec::EncodeCuda(uint8_t* &pHostFrame, int n_cam_index)
    {
  
      
        ck(cuCtxSetCurrent((CUcontext)pEncsCuda_[n_cam_index]->GetDevice()));
        // oDeviceSrc->copyFrom(pHostFrame, oDeviceSrc->pitch());
        // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(oDeviceSrc->data(), (int)oDeviceSrc->width(), {(int)oDeviceSrc->width(), (int)oDeviceSrc->height()}, 
        //                      {0, 0, (int)oDeviceSrc->width(),(int)oDeviceSrc->height() }, (Npp8u *)oDeviceDest->data(), (int)oDeviceDest->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, 100);
        
        
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        //  bayer_device_srcs_[n_cam_index]->copyFromAsync(pHostFrame, bayer_device_srcs_[n_cam_index]->pitch(), npp_stream_contextes_[n_cam_index].hStream);
        bayer_device_srcs_[n_cam_index]->copyFrom(pHostFrame, bayer_device_srcs_[n_cam_index]->pitch());
        
        // std::cout<<"pitch Dest:"<< rgb_device_dst_.pitch()<<std::endl;
        // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R_Ctx(bayer_device_srcs_[n_cam_index]->data(), (int)bayer_device_srcs_[n_cam_index]->width(), {(int)bayer_device_srcs_[n_cam_index]->width(), (int)bayer_device_srcs_[n_cam_index]->height()}, 
        //                     {0, 0, (int)bayer_device_srcs_[n_cam_index]->width(),(int)bayer_device_srcs_[n_cam_index]->height() }, (Npp8u *)rgba_device_dsts_[n_cam_index]->data(), (int)rgba_device_dsts_[n_cam_index]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100, npp_stream_contextes_[n_cam_index]);
        
        NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(bayer_device_srcs_[n_cam_index]->data(), (int)bayer_device_srcs_[n_cam_index]->width(), {(int)bayer_device_srcs_[n_cam_index]->width(), (int)bayer_device_srcs_[n_cam_index]->height()}, 
                            {0, 0, (int)bayer_device_srcs_[n_cam_index]->width(),(int)bayer_device_srcs_[n_cam_index]->height() }, (Npp8u *)rgba_device_dsts_[n_cam_index]->data(), (int)rgba_device_dsts_[n_cam_index]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, 100);
        
        // cv::Mat bayer8BitMat(height_, width_, CV_8UC1, pHostFrame);
        // cv::Mat rgba8BitMat(height_, width_, CV_8UC4);
        // cv::cvtColor(bayer8BitMat, rgba8BitMat, cv::COLOR_BayerRG2RGBA);
        
        
        // cudaError_t cudaResult = cudaStreamSynchronize(npp_stream_contextes_[n_cam_index].hStream);
            // ENSURE(cudaSuccess == cudaResult);

       // cudaDeviceSynchronize();
        // For receiving encoded packets
        std::vector<std::vector<uint8_t>> vPacket;

        if (!exit_flag.load(std::memory_order_acquire))
        {
            const NvEncInputFrame* encoderInputFrame = pEncsCuda_[n_cam_index]->GetNextInputFrame();
            
            clock_t start = clock();
                // NvEncoderCuda::CopyToDeviceFrame((CUcontext)pEncsCuda_[n_cam_index]->GetDevice(), rgba8BitMat.data, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
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


            
            pEncsCuda_[n_cam_index]->EncodeFrame(vPacket);
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
       
        if (exit_flag.load(std::memory_order_acquire)) return ;
    


    }

    // void  BayerToH264ConverterNvidiaCodec::EncodeCudaOpInVidMem(uint8_t* &pHostFrame, int n_cam_index) {

    //       bayer_device_srcs_[n_cam_index]->copyFromAsync(pHostFrame, bayer_device_srcs_[n_cam_index]->pitch());
        
    //     // std::cout<<"pitch Dest:"<< rgb_device_dst_.pitch()<<std::endl;
    //     // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R_Ctx(bayer_device_srcs_[n_cam_index]->data(), (int)bayer_device_srcs_[n_cam_index]->width(), {(int)bayer_device_srcs_[n_cam_index]->width(), (int)bayer_device_srcs_[n_cam_index]->height()}, 
    //     //                     {0, 0, (int)bayer_device_srcs_[n_cam_index]->width(),(int)bayer_device_srcs_[n_cam_index]->height() }, (Npp8u *)rgba_device_dsts_[n_cam_index]->data(), (int)rgba_device_dsts_[n_cam_index]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100, npp_stream_contextes_[n_cam_index]);
        
    //      NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(bayer_device_srcs_[n_cam_index]->data(), (int)bayer_device_srcs_[n_cam_index]->width(), {(int)bayer_device_srcs_[n_cam_index]->width(), (int)bayer_device_srcs_[n_cam_index]->height()}, 
    //                         {0, 0, (int)bayer_device_srcs_[n_cam_index]->width(),(int)bayer_device_srcs_[n_cam_index]->height() }, (Npp8u *)rgba_device_dsts_[n_cam_index]->data(), (int)rgba_device_dsts_[n_cam_index]->width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, 100);
    //     // return;

    //     std::vector<NV_ENC_OUTPUT_PTR> pVideoMemBfr;
    //      if (!exit_flag.load(std::memory_order_acquire))
    //     {
    //         const NvEncInputFrame* encoderInputFrame = pEncsVidCuda_[n_cam_index]->GetNextInputFrame();

    //         NvEncoderCuda::CopyToDeviceFrame(cu_contexts_[n_cam_index], rgba_device_dsts_[n_cam_index]->data(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
	// 			(int)encoderInputFrame->pitch,
	// 			pEncsVidCuda_[n_cam_index]->GetEncodeWidth(),
	// 			pEncsVidCuda_[n_cam_index]->GetEncodeHeight(),
	// 			CU_MEMORYTYPE_DEVICE,
	// 			encoderInputFrame->bufferFormat,
	// 			encoderInputFrame->chromaOffsets,
	// 			encoderInputFrame->numChromaPlanes,
	// 			true,
	// 			p_cu_streams_[n_cam_index]->GetInputCUStream());

	// 		pEncsVidCuda_[n_cam_index]->EncodeFrame(pVideoMemBfr);
    //     }
    //      else
    //     {
    //         pEncsVidCuda_[n_cam_index]->EndEncode(pVideoMemBfr);
    //     }
    //     for (uint32_t i = 0; i < pVideoMemBfr.size(); ++i)
	// 	{
			

	// 	//	pDumpVidMemOutput->DumpOutputToFile((CUdeviceptr)(pVideoMemBfr[i]), bUseCUStream ? 0 : 0, fpOut, nFrame);

	// 	}

       
    //     if (exit_flag.load(std::memory_order_acquire)) return ;
    // }

   
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
    

    
    
    void  BayerToH264ConverterNvidiaCodec::initialize(){
        
        std::ostringstream oss;
        oss<<"-fps "<<fps_<<" -cq 31";

        encode_CLI_options_ = NvEncoderInitParam(oss.str().c_str());

        // cu_contexts_.resize(num_devices_, NULL);

        ck(cuInit(0));
		int nGpu = 0;
        int iGpu = 0;
		ck(cuDeviceGetCount(&nGpu));
		if (iGpu < 0 || iGpu >= nGpu)
		{
			std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
			return ;
		}


        CUdevice cuDevice = 0;
		ck(cuDeviceGet(&cuDevice, iGpu));
		char szDeviceName[80];
		ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
		std::cout << "GPU in use: " << szDeviceName << std::endl;
        
        // ck(cuCtxCreate(&cu_context_, 0, cuDevice));

        cuda_streams_.resize(num_devices_, nullptr);
        npp_stream_contextes_.resize(num_devices_, {});
        CUcontext cuContext = NULL;
        // fp_outs_.resize(num_devices_);
        for (unsigned int i = 0 ; i < num_devices_; i++)
        {
            
           
            ck(cuCtxCreate(&cuContext, 0, cuDevice));
            
            
            
            
            bayer_device_srcs_.push_back(std::make_unique<npp::ImageNPP_8u_C1>(width_, height_, true));
            bayer_dp_buf_vec_.push_back((CUdeviceptr)bayer_device_srcs_.back()->data());
            rgba_device_dsts_.push_back(std::make_unique<npp::ImageNPP_8u_C4> (width_, height_, true));
            rgba_dp_buf_vec_.push_back((CUdeviceptr)rgba_device_dsts_.back()->data());

            cudaError_t cudaResult = cudaStreamCreateWithFlags(&cuda_streams_[i], cudaStreamDefault);
            ENSURE(cudaSuccess == cudaResult);
            NppStatus nppStatus = nppGetStreamContext(&npp_stream_contextes_[i]);
            ENSURE(NPP_SUCCESS == nppStatus);
            npp_stream_contextes_[i].hStream = cuda_streams_[i];
            
            // ck(cuCtxCreate(&cu_contexts_[i], 0, cuDevice));
            // // cuCtxPushCurrent(cu_contexts_[i]);
            // cuCtxPopCurrent(&prev_context);
            
            std::string file_name = "Dev_" + map_serial_nums_[i]  + ".mp4" ;
            fp_outs_.push_back(std::ofstream(file_name, std::ios::out | std::ios::binary));
            
            NvEncPtr pEnc(new NvEncoderCuda(cuContext, width_, height_, enc_format_), EncodeDeleteFunc);
           
            // pEncsVidCuda_.push_back(std::make_unique< NvEncoderOutputInVidMemCuda>(cu_contexts_[i], width_, height_, enc_format_));

            InitializeEncoder(pEnc, encode_CLI_options_, enc_format_);
            // p_cu_streams_.push_back(std::make_unique<NvCUStream>(reinterpret_cast<CUcontext>(pEncsVidCuda_.back()->GetDevice()), 1, pEncsVidCuda_.back()));
            
            pEncsCuda_.push_back(std::move(pEnc));
           
        }

    
            
        

    }
    

  