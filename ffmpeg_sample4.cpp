// #include <iostream>
// #include <fstream>
// #include <vector>
// extern "C" {
// #include <libavformat/avformat.h>
// #include <libavcodec/avcodec.h>
// #include <libavutil/hwcontext.h>
// #include <libavutil/opt.h>
// #include <libavutil/imgutils.h> // Include this header

// }

// int main(int argc, char* argv[]) {
//     // Open the input file
//     // std::ifstream input_file("bayer8.bin", std::ios::in | std::ios::binary);
//     // std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
//     FILE * input_file = fopen("/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", "rb");

//     // std::cout<<"size:"<<input_data.size()<<std::endl;

//     // Set up the output format and codec
//     AVFormatContext* output_format_context = nullptr;
//     avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, "output.mp4");
//     AVStream* output_stream = avformat_new_stream(output_format_context, nullptr);
//     AVCodec* output_codec = avcodec_find_encoder_by_name("h264_nvenc");
//     if (!output_codec) {
//         std::cerr << "Failed to find NVIDIA CUDA H.264 encoder" << std::endl;
//         return 1;
//     }
//     AVCodecContext* output_codec_context = avcodec_alloc_context3(output_codec);
//     output_codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
//     output_codec_context->pix_fmt = AV_PIX_FMT_RGBA;
//     output_codec_context->width = 4096;
//     output_codec_context->height = 3000;
//     output_codec_context->time_base = AVRational{1, 9};
//     output_codec_context->framerate = AVRational{9, 1};
//     av_opt_set(output_codec_context->priv_data, "hwaccel", "cuda", 0);
//     av_opt_set(output_codec_context->priv_data, "hwaccel_output_format", "cuda", 0);
//     avcodec_open2(output_codec_context, output_codec, nullptr);
//     avcodec_parameters_from_context(output_stream->codecpar, output_codec_context);

//     avio_open(&output_format_context->pb, "output.mp4", AVIO_FLAG_WRITE);
//     avformat_write_header(output_format_context, nullptr);

//     // Encode the frames
//     // AVPacket* packet = av_packet_alloc();
//     // AVFrame* frame = av_frame_alloc();
//     // frame->format = AV_PIX_FMT_BAYER_RGGB8;
//     // frame->width = 4096;
//     // frame->height = 3000;
//     // av_image_fill_arrays(frame->data, frame->linesize, input_file, AV_PIX_FMT_BAYER_RGGB8, 4096, 3000, 1);
//     int ret;
//     // int frame_count = input_data.size() / (4096 * 3000);
//     int width = 4096, height = 3000; // Update these values as per your raw file
//     int frame_size = width * height * 1; 
//     uint8_t *frame_data = (uint8_t*)av_malloc(frame_size);
//     int i = 0;
//     while (fread(frame_data, 1, frame_size, input_file) == frame_size) {
//     // for (int i = 0; i < frame_count; ++i) {
//         // std::cout<<"i:"<<i<<" "<<frame_count<<std::endl;
//         AVFrame* frame = av_frame_alloc();
//         frame->format = AV_PIX_FMT_BAYER_RGGB8;
//         frame->width = width;
//         frame->height = height;
//         av_image_fill_arrays(frame->data, frame->linesize, frame_data, AV_PIX_FMT_BAYER_RGGB8, 4096, 3000, 1);

//         frame->pts = i++;
//         ret = avcodec_send_frame(output_codec_context, frame);
//         if (ret < 0) {
//             std::cerr << "Failed to send frame to encoder: " << (ret) << std::endl;
//             break;
//         }
//         AVPacket* packet = av_packet_alloc();

//         while (ret >= 0) {
//             ret = avcodec_receive_packet(output_codec_context, packet);
//             if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//                 break;
//         } else if (ret < 0) {
//             std::cerr << "Failed to receive packet from encoder: " << (ret) << std::endl;
//             break;
//         }

//         av_packet_rescale_ts(packet, output_codec_context->time_base, output_stream->time_base);
//         packet->stream_index = output_stream->index;
//         av_write_frame(output_format_context, packet);
//         av_packet_unref(packet);
//         }
//          av_frame_free(&frame);
//         // av_packet_free(&packet);
//     }

//         // Flush the encoder
//         // avcodec_send_frame(output_codec_context, nullptr);
//         // while (avcodec_receive_packet(output_codec_context, packet) >= 0) {
//         // av_packet_rescale_ts(packet, output_codec_context->time_base, output_stream->time_base);
//         // packet->stream_index = output_stream->index;
//         // av_write_frame(output_format_context, packet);
//         // }

//         // Write the trailer and clean up
//         av_write_trailer(output_format_context);
//         avcodec_free_context(&output_codec_context);
//         avformat_free_context(output_format_context);
//         // av_frame_free(&frame);
//         // av_packet_free(&packet);

//         return 0;
//         }

// #include <iostream>
// #include <fstream>
// #include <vector>
// extern "C" {
// #include <libavformat/avformat.h>
// #include <libavcodec/avcodec.h>
// #include <libavutil/hwcontext.h>
// #include <libavutil/opt.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/pixdesc.h>
// }
// #include <nppi.h>
// #include <nppcore.h>
// #include <cuda.h>


// int main(int argc, char* argv[]) {
//     // Open the input file
//     // std::ifstream input_file("bayer8.bin", std::ios::binary);
//     // std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
//     FILE * input_file = fopen("/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", "rb");

//     // Set up the output format and codec
//     AVFormatContext* output_format_context = nullptr;
//     avformat_alloc_output_context2(&output_format_context, nullptr, nullptr, "output.mp4");
//     AVStream* output_stream = avformat_new_stream(output_format_context, nullptr);
//     AVCodec* output_codec = avcodec_find_encoder_by_name("h264_nvenc");
//     if (!output_codec) {
//         std::cerr << "Failed to find NVIDIA CUDA H.264 encoder" << std::endl;
//         return 1;
//     }
//     AVCodecContext* output_codec_context = avcodec_alloc_context3(output_codec);
//     output_codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
//     output_codec_context->pix_fmt = AV_PIX_FMT_RGB0;
//     output_codec_context->width = 4096;
//     output_codec_context->height = 3000;
//     output_codec_context->time_base = AVRational{1, 9};
//     output_codec_context->framerate = AVRational{9, 1};
//     av_opt_set(output_codec_context->priv_data, "hwaccel", "cuda", 0);
//     av_opt_set(output_codec_context->priv_data, "hwaccel_output_format", "cuda", 0);
//     avcodec_open2(output_codec_context, output_codec, nullptr);
//     avcodec_parameters_from_context(output_stream->codecpar, output_codec_context);

//     avio_open(&output_format_context->pb, "output.mp4", AVIO_FLAG_WRITE);
//     avformat_write_header(output_format_context, nullptr);

// // Set up NPP
//     NppStreamContext npp_stream_ctx;
//     nppStreamCreate(&npp_stream_ctx);
//     nppSetActiveMaskStream(npp_stream_ctx);


//     // Set up the CUDA context and frame
//     AVBufferRef* hw_device_ctx = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_CUDA);
//     if (!hw_device_ctx) {
//         std::cerr << "Failed to create CUDA device context" << std::endl;
//         return 1;
//     }
//     AVHWDeviceContext* device_ctx = (AVHWDeviceContext*)hw_device_ctx->data;
//     int ret = av_hwdevice_ctx_init(hw_device_ctx);
//     if (ret < 0) {
//         // std::string err_str ((ret));
//         std::cerr << "Failed to initialize CUDA device context: " << (ret) << std::endl;
//         return 1;
//     }

//     AVBufferRef* hw_frame_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
//     if (!hw_frame_ctx) {
//         std::cerr << "Failed to create CUDA frame context" << std::endl;
//         return 1;
//     }
//     AVHWFramesContext* hw_frames_ctx = (AVHWFramesContext*)hw_frame_ctx->data;
// hw_frames_ctx->format = AV_PIX_FMT_BAYER_RGGB8;
// hw_frames_ctx->width = 4096;
// hw_frames_ctx->height = 3000;
// hw_frames_ctx->initial_pool_size = 8;
// hw_frames_ctx->device_ctx = device_ctx;
// ret = av_hwframe_ctx_init(hw_frame_ctx);
// if (ret < 0) {
//     std::cerr << "Failed to initialize CUDA frame context: " << (ret) << std::endl;
//     return 1;
// }

// AVFrame* hw_frame = av_frame_alloc();
// ret = av_hwframe_get_buffer(hw_frame_ctx, hw_frame, 0);
// if (ret < 0) {
//     std::cerr << "Failed to get hardware frame: " << (ret) << std::endl;
//     return 1;
// }

// // Encode the frames
// AVPacket* packet = av_packet_alloc();
// // int frame_count = input_data.size() / (4096 * 3000);
// int width = 4096, height = 3000; // Update these values as per your raw file
// int frame_size = width * height * 1; 
// uint8_t *frame_data = (uint8_t*)av_malloc(frame_size);
// int i = 0;
// while (fread(frame_data, 1, frame_size, input_file) == frame_size) {


// // for (int i = 0; i < frame_count; ++i) {
//     AVFrame* bayer_frame = av_frame_alloc();
//     bayer_frame->format = AV_PIX_FMT_BAYER_RGGB8;
//     bayer_frame->width = 4096;
//     bayer_frame->height = 3000;
//     av_image_fill_arrays(bayer_frame->data, bayer_frame->linesize, frame_data, AV_PIX_FMT_BAYER_RGGB8, 4096, 3000, 1);

//     // ret = av_hwframe_transfer_data(hw_frame, bayer_frame, &hw_frames_ctx);
//     ret = av_hwframe_transfer_data(hw_frame, bayer_frame, 0);

//     if (ret < 0) {
//         std::cerr << "Failed to transfer data to GPU: " << (ret) << std::endl;
//         break;
//     }
//     av_frame_free(&bayer_frame);

//     hw_frame->pts = i++;
//     ret = avcodec_send_frame(output_codec_context, hw_frame);
//     if (ret < 0) {
//         std::cerr << "Failed to send frame to encoder: " << (ret) << std::endl;
//         break;
//     }

//     while (ret >= 0) {
//         ret = avcodec_receive_packet(output_codec_context, packet);
//         if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
//             break;
//         } else if (ret < 0) {
//             std::cerr << "Failed to receive packet from encoder: " << (ret) << std::endl;
//             break;
//         }

//         av_packet_rescale_ts(packet, output_codec_context->time_base, output_stream->time_base);
//         packet->stream_index = output_stream->index;
//         av_write_frame(output_format_context, packet);
//         av_packet_unref(packet);
//     }
// }

// // Flush the encoder
// avcodec_send_frame(output_codec_context, nullptr);
// while (avcodec_receive_packet(output_codec_context, packet) >= 0) {
//     av_packet_rescale_ts(packet, output_codec_context->time_base, output_stream->time_base);
//     packet->stream_index = output_stream->index;
//     av_write_frame(output_format_context, packet);
//     av_packet_unref(packet);
// }

// // Write the trailer and clean up
// av_write_trailer(output_format_context);
// avcodec_free_context(&output_codec_context);
// avformat_free_context(output_format_context);
// av_frame_free(&hw_frame);
// av_packet_free(&packet);
// av_buffer_unref(&hw_frame_ctx);
// av_buffer_unref(&hw_device_ctx);

// return 0;
// }

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <nppi.h>
#include <nppcore.h>
#include <ImageIO.h>
#include <ImagesCPU.h>
#include <ImagesNPP.h>
#include <fstream>
#include "tiffio.h"
#include <png.h>
#include <opencv2/highgui.hpp>
// #include <opencv2/cudaimgproc.hpp>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
}


static AVBufferRef *hw_device_ctx = NULL;

static int hw_encode_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}


void save_rgba_to_png(const std::string& filename, const unsigned char* rgba_buffer, int width, int height) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Failed to open file for writing");
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        throw std::runtime_error("Failed to create PNG write structure");
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        throw std::runtime_error("Failed to create PNG info structure");
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        throw std::runtime_error("Error during PNG creation");
    }

    png_init_io(png, fp);

    // Write header (8 bit color depth)
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // Write image data
    for (int y = 0; y < height; y++) {
        png_write_row(png, rgba_buffer + y * width * 3);
    }

    // End write
    png_write_end(png, nullptr);

    fclose(fp);
    png_destroy_write_struct(&png, &info);
}



static bool SaveBayerAsTiff(const std::string &file_name, uint8_t *buffer,  uint32_t width, u_int32_t height)
{
    
    // cv::Mat bayer_image(height, width, CV_8UC1, buffer);
    // cv::Mat rgb_image;
    // cv::cvtColor(bayer_image, rgb_image, cv::COLOR_BayerRG2BGR);

    // uint8_t* rgb_buffer = (uint8_t*)std::malloc(width * height * 3 * sizeof(uint8_t));
    //  // Convert RGBA to RGB
    // for (uint32_t y = 0; y < height; ++y) {
    //     for (uint32_t x = 0; x < width; ++x) {
    //         uint32_t rgba_index = (y * width + x) * 4;
    //         uint32_t rgb_index = (y * width + x) * 3;
    //         rgb_buffer[rgb_index + 0] = buffer[rgba_index + 0]; // Red
    //         rgb_buffer[rgb_index + 1] = buffer[rgba_index + 1]; // Green
    //         rgb_buffer[rgb_index + 2] = buffer[rgba_index + 2]; // Blue
    //     }
    // }


    TIFF* tif = TIFFOpen(file_name.c_str(), "w");
    if (tif) {
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);  // Bayer8 image
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);  // or PHOTOMETRIC_MINISWHITE

        u_int8_t *scan_line = (u_int8_t *)malloc(width*(sizeof(u_int8_t)));
            // Write the image data
        for (u_int8_t i = 0; i < height; ++i) {

             memcpy(scan_line, &buffer[i*width], width * sizeof(uint32_t));
            TIFFWriteScanline(tif, scan_line, i, 0);
            // if (TIFFWriteScanline(tif, &buffer[row * width * 4], row, 0) < 0) {
            //     std::cerr << "Could not write scanline " << row << std::endl;
            //     TIFFClose(tif);
            //     // std::free(rgb_buffer);
            //     return 1;
            // }
        }

        free(scan_line);
        // for (int row = 0; row < height; row++) {
        //     TIFFWriteScanline(tif, rgb_image.ptr(row), row);
        // }

        TIFFClose(tif);
        //  std::free(rgb_buffer);


    } else return false;

    
    return true;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
    // fwrite(pFrame->data[0], 1, width * height, pFile);
    // fwrite(pFrame->data[1], 1, width * height / 4, pFile);
    // fwrite( pFrame->data[2], 1,  width * height /4, pFile);
    // Write Y plane
    for (int i = 0; i < height; i++) {
        fwrite(pFrame->data[0] + i * pFrame->linesize[0], 1, width, pFile);
    }

    // Write U plane
    for (int i = 0; i < height / 2; i++) {
        fwrite(pFrame->data[1] + i * pFrame->linesize[1], 1, width / 2, pFile);
    }

    // Write V plane
    for (int i = 0; i < height / 2; i++) {
        fwrite(pFrame->data[2] + i * pFrame->linesize[2], 1, width / 2, pFile);
    }

//   for(y=0; y<height; y++)
//     fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  // Close file
  fclose(pFile);
}
void save_buffer_as_ppm(int width, int height, int iFrame) {
    char szFilename[32];
    sprintf(szFilename, "frame%d.ppm", iFrame);
    FILE *fp = fopen(szFilename, "wb");
    if (!fp) {
    fprintf(stderr, "Error opening file: %s\n", szFilename);
    return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", width, height);

}


void saveRGBABufferToPPM(const unsigned char* buffer, int width, int height, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Write the PPM header
    file << "P6" << std::endl;
    file << width << " " << height << std::endl;
    file << "255" << std::endl;

    // // Write the pixel data
    // for (int i = 0; i <  width * height * 4; i += 4) {
    //     file << buffer[i + 0]; // Red
    //     file << buffer[i + 1]; // Green
    //     file << buffer[i + 2]; // Blue
    //     // file << buffer[i + 4]; // Alpha
    // }

    for (int i = 0; i < width * height; ++i) {
        unsigned char r = buffer[i * 4 + 0];
        unsigned char g = buffer[i * 4 + 1];
        unsigned char b = buffer[i * 4 + 2];
        unsigned char a = buffer[i * 4 + 3];

        // Premultiply the RGB values by the alpha value
        // r = static_cast<unsigned char>(static_cast<float>(r) * (a / 255.0f));
        // g = static_cast<unsigned char>(static_cast<float>(g) * (a / 255.0f));
        // b = static_cast<unsigned char>(static_cast<float>(b) * (a / 255.0f));

        file << r << g << b;
    }

    file.close();
    std::cout << "RGBA buffer saved to " << filename << " as PPM file." << std::endl;
}


void save_frame_as_ppm(AVFrame *frame,  int iFrame) {
    char szFilename[32];

    sprintf(szFilename, "frame%d.ppm", iFrame);
    FILE *fp = fopen(szFilename, "wb");
    if (!fp) {
        fprintf(stderr, "Error opening file: %s\n", szFilename);
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", frame->width, frame->height);

    for (int y = 0; y < frame->height; y++) {
        fwrite(frame->data[0] + y * frame->linesize[0], 1, frame->width * 3, fp);
    }

    fclose(fp);
}

static int set_hwframe_ctx(AVCodecContext *ctx, AVBufferRef *hw_device_ctx, int width, int height)
{
    AVBufferRef *hw_frames_ref;
    AVHWFramesContext *frames_ctx = NULL;
    int err = 0;

    if (!(hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx))) {
        fprintf(stderr, "Failed to create VAAPI frame context.\n");
        return -1;
    }
    frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = AV_PIX_FMT_RGB0;
    frames_ctx->width     = width;
    frames_ctx->height    = height;
    // frames_ctx->initial_pool_size = 20;
    if ((err = av_hwframe_ctx_init(hw_frames_ref)) < 0) {
        fprintf(stderr, "Failed to initialize VAAPI frame context."
                "Error code: %d\n",(err));
        av_buffer_unref(&hw_frames_ref);
        return err;
    }
    ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
    if (!ctx->hw_frames_ctx)
        err = AVERROR(ENOMEM);

    av_buffer_unref(&hw_frames_ref);
    return err;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }
 


    // nppiConvert_8u32f_C3R
    // Initialize FFmpeg
    // av_register_all();
    // avcodec_register_all();

    // Open the input file


    FILE *input_file = fopen("/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", "rb");
    if (!input_file) {
        throw std::runtime_error("Failed to open input file");
    }

    // Get the input file dimensions
    int width = 4096;
    int height = 3000;
    const AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
    // const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec  not found\n");
        exit(1);
    }


    // Create the output video stream
    AVFormatContext *output_ctx = nullptr;
    // if (avformat_alloc_output_context2(&output_ctx, nullptr, "mp4", "/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/yuv_output.mp4") < 0) {
    //     throw std::runtime_error("Failed to create output format context");
    // }
    int ret = avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, argv[1]);
    if (ret < 0) 
    {
        fprintf(stderr, "Error creating output context\n");
        return -1;
    }
    AVStream *output_stream = avformat_new_stream(output_ctx, nullptr);
    if (!output_stream) {
        throw std::runtime_error("Failed to create output stream");
    }

    // Set the output video codec and parameters
    // output_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    // output_stream->codecpar->width = width;
    // output_stream->codecpar->height = height;
    // output_stream->codecpar->format = AV_PIX_FMT_RGBA;

   

    // // Open the output file
    // if (avio_open(&output_ctx->pb,  argv[1], AVIO_FLAG_WRITE) < 0) {
    //     throw std::runtime_error("Failed to open output file");
    // }

    // Set up hardware acceleration
    AVCodecContext *output_codec_ctx = avcodec_alloc_context3(codec);
    if (!output_codec_ctx) {
        throw std::runtime_error("Failed to allocate output codec context");
    }
    // enum AVHWDeviceType type;

    // type = av_hwdevice_find_type_by_name("cuda");

    // if (type == AV_HWDEVICE_TYPE_NONE) {
    //     fprintf(stderr, "Device type %s is not supported.\n", "cuda");
    //     fprintf(stderr, "Available device types:");
    //     while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
    //         fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
    //     fprintf(stderr, "\n");
    //     return -1;
    // }


    //  if (hw_encode_init(output_codec_ctx, type) < 0)
    //         return -1 ;



    // output_codec_ctx->codec_id = AV_CODEC_ID_H264;
    output_codec_ctx->width = width;
    output_codec_ctx->height = height;
    output_codec_ctx->pix_fmt = AV_PIX_FMT_RGB0;

    output_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;

    output_codec_ctx->time_base = {1,9} ;
    output_codec_ctx->framerate = {9,1}; 
    // output_codec_ctx->gop_size = 10;
    // output_codec_ctx->bit_rate = 5000000;
    output_codec_ctx->max_b_frames = 1;

    // ret = av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0);
    // if (ret < 0) {
    //     fprintf(stderr, "Failed to create a VAAPI device. Error code: %d\n", (ret));
    //     return -1;
    // }
    // int err;
    // if ((err = set_hwframe_ctx(output_codec_ctx, hw_device_ctx, width, height)) < 0) {
    //     fprintf(stderr, "Failed to set hwframe context.\n");
    //     return -1;
    // }



    

    // AVBufferRef *hw_device_ctx = NULL;
	// av_hwdevice_ctx_create(&hw_device_ctx, type,
	//       NULL, NULL, 0); 

    // AVBufferRef *hw_frames_ref = NULL;
	// hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);

    // AVBufferRef* hw_frame_ctx = av_hwframe_ctx_alloc(hw_device_ctx);
    // AVHWFramesContext *frames_ctx = (AVHWFramesContext *)(hw_frame_ctx->data);
    // frames_ctx->format = AV_PIX_FMT_CUDA;
    // frames_ctx->sw_format = AV_PIX_FMT_RGB24;
    // frames_ctx->width = width;
    // frames_ctx->height = height;
    // frames_ctx->initial_pool_size = 20;
    // av_hwframe_ctx_init(hw_frame_ctx);

    // AVHWFramesContext *frames_ctx; 
	// frames_ctx = (AVHWFramesContext *)(hw_frames_ref->data);
	// frames_ctx->format    = AV_PIX_FMT_CUDA;
	// frames_ctx->sw_format = AV_PIX_FMT_BAYER_RGGB8;
	// frames_ctx->width     = width;
	// frames_ctx->height    = height;

	// av_hwframe_ctx_init(hw_frames_ref);

    // output_codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);


    // if (codec->id == AV_CODEC_ID_H264)
    //     av_opt_set(output_codec_ctx->priv_data, "preset", "veryfast", 0);


    // av_opt_set(output_codec_ctx->priv_data, "hwaccel", "cuda", 0);
    // av_opt_set(output_codec_ctx->priv_data, "hwaccel_output_format", "cuda", 0);
    if (avcodec_open2(output_codec_ctx, codec, nullptr) < 0) 
    {
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }
    avcodec_parameters_from_context(output_stream->codecpar, output_codec_ctx);

    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) 
    {
        if (avio_open(&output_ctx->pb, argv[1], AVIO_FLAG_WRITE) < 0) 
        {
            fprintf(stderr, "Could not open output file '%s'\n", argv[1]);
            return -1;
        }
    }

    if (avformat_write_header(output_ctx, nullptr) < 0) 
    {
        fprintf(stderr, "Error writing header\n");
        return -1;
    }
  

    // Allocate frames and set up the scaler
    AVFrame *input_frame = av_frame_alloc();
    input_frame->width = width;
    input_frame->height = height;
    input_frame->format = AV_PIX_FMT_BAYER_RGGB8;

    if (av_frame_get_buffer(input_frame, 0) < 0) {
        throw std::runtime_error("Failed to allocate input frame buffer");
    }

    
    // AVFrame* gpu_frame = av_frame_alloc();
    // if (!gpu_frame) {
    //     throw std::runtime_error("Could not allocate frame");
    // }

    // // Set the frame properties
    // gpu_frame->format = AV_PIX_FMT_CUDA;
    // gpu_frame->width  = width;
    // gpu_frame->height = height;

    // Allocate GPU frame buffer
    // if (av_hwframe_get_buffer(output_codec_ctx->hw_frames_ctx, cuda_frame, 0) < 0) {
    //     throw std::runtime_error("Could not allocate GPU frame");
    // }
  
   
   
    AVFrame *output_frame = av_frame_alloc();

    output_frame->width = width;
    output_frame->height = height;
    output_frame->format = AV_PIX_FMT_RGBA;
  
    if (av_frame_get_buffer(output_frame, 0) < 0) {
        throw std::runtime_error("Failed to allocate output frame buffer");
    }

    SwsContext *sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_BAYER_RGGB8, 
        width, height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

   

    uint8_t *frame_data = (uint8_t*)av_malloc(width * height);
    // Process the raw Bayer8 frames
    int i =0;
    while (fread(frame_data, 1, width * height, input_file)) {
        // Read a Bayer8 frame from the input file
        // size_t bytes_read = fread(frame_data, 1, width * height, input_file);
        // if (bytes_read != width * height) {
        //     break;
        // }
        // if (i>20) break;
        // Set the input frame properties
        // clock_t start = clock();

        int ret = av_image_fill_arrays(
            input_frame->data, input_frame->linesize,
            frame_data, AV_PIX_FMT_BAYER_RGGB8,
            width, height, 1
        );


        //  int ret = av_image_fill_arrays(
        //     input_frame->data, input_frame->linesize,
        //     frame_data_dumy, AV_PIX_FMT_BAYER_RGGB8,
        //     1, 1, 1
        // );
        //  if (!(output_frame = av_frame_alloc())) {
        //     ret = AVERROR(ENOMEM);
        //     return -1;
        // }

       
        // input_frame->data[0] = frame_data;
        // input_frame->linesize[0] = width;

        clock_t start = clock();

        // Use hardware-accelerated scaling to convert the Bayer8 frame to YUV420P
        // av_hwframe_transfer_data(output_frame, input_frame, 0);
        // sws_scale(sws_ctx, (const uint8_t * const *)output_frame->data, output_frame->linesize, 0, height, output_frame->data, output_frame->linesize);
        sws_scale(sws_ctx, (const uint8_t * const *)input_frame->data, input_frame->linesize, 0, height, output_frame->data, output_frame->linesize);
        // clock_t end = clock();
        // double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
        // printf("time taken by sws_scale in seconds : %f\n", elapsed);

        // npp::ImageCPU_8u_C1 oHostSrc(width, height);
        // npp::loadImage(std::string("./output.bmp"), oHostSrc);
        // std::cout<<"pitch:"<< oHostSrc.pitch()<<std::endl;

        // npp::ImageNPP_8u_C1 oDeviceSrc(width, height, true);
        // oDeviceSrc.copyFrom(frame_data, oDeviceSrc.pitch());
        // // std::cout<<"pitch:"<< oDeviceSrc.pitch()<<std::endl;



        // // npp::ImageNPP_8u_C3 oDeviceDest(width, height);
        // npp::ImageNPP_8u_C4 oDeviceDest(width, height, true);
        // // std::cout<<"pitch Dest:"<< oDeviceDest.pitch()<<std::endl;


        // NppStatus stat = nppiCFAToRGBA_8u_C1AC4R(oDeviceSrc.data(), (int)oDeviceSrc.width(), {(int)oDeviceSrc.width(), (int)oDeviceSrc.height()}, 
        //                     {0, 0, (int)oDeviceSrc.width(),(int)oDeviceSrc.height() }, (Npp8u *)oDeviceDest.data(), (int)oDeviceDest.width()*4, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED, (Npp8u)100);
       
        // // NppStatus stat = nppiCFAToRGB_8u_C1C3R(oDeviceSrc.data(), (int)oDeviceSrc.width(), {(int)oDeviceSrc.width(), (int)oDeviceSrc.height()}, 
        // //                     {0, 0, (int)oDeviceSrc.width(),(int)oDeviceSrc.height() }, (Npp8u *)oDeviceDest.data(), (int)oDeviceDest.width()*3, NPPI_BAYER_RGGB, NPPI_INTER_UNDEFINED);
       
        
        // // Npp8u *pDst[3];
        
        // // int yuv_step;
        // // stat = nppiRGBToYUV420_8u_C3P3R(oDeviceDest.data(), (int)oDeviceDest.width()*3, pDst, yuv_step)
        // npp::ImageCPU_8u_C4 oHostData(width, height);
        // // npp::ImageCPU_8u_C3 oHostData(width, height);

        // oDeviceDest.copyTo((Npp8u*)oHostData.data(), oHostData.pitch());



        // std::cout<<"pitch DEvice:"<< /*oDeviceDest.data()<< " "<< oHostData.data()<<std::endl;

        // npp::saveImage("frame_"+std::to_string(i)+".png", oHostData);
        // saveRGBABufferToPPM(oHostData.data(), width, height, "frame_"+std::to_string(i)+".png");

        // save_rgba_to_png("frame_"+std::to_string(i)+".png",oHostData.data(),  width, height);
        // SaveBayerAsTiff("frame_"+std::to_string(i)+".tiff", oHostData.data(), width, height);
     

        // cv::Mat bayer_cpu(height, width, CV_8UC1, frame_data);
        // cv::cuda::GpuMat bayer_gpu;
        // bayer_gpu.upload(bayer_cpu);

        // cv::cuda::GpuMat rgba_gpu;
        // cv::Mat rgba_cpu;
        // // rgba_gpu.create(height, width, CV_8UC4);
        // // cv::cuda::demosaicing(bayer_gpu, rgba_gpu, cv::COLOR_BayerRGGB2RGB);
        // // cv::cuda::cvtColor(bayer_gpu, rgba_gpu, cv::COLOR_BayerRGGB2RGB);
        // // cv::cuda::GpuMat yuv_gpu;//(height, width, CV_8UC3);
        // // cv::cuda::cvtColor(rgba_gpu, yuv_gpu, cv::COLOR_RGBA2YUV_I420);

        // cv::cvtColor(bayer_cpu, rgba_cpu, cv::COLOR_BayerRGGB2RGBA);
    
        // // rgba_gpu.download(rgba_cpu);
        // rgba_gpu.upload(rgba_cpu);
        // clock_t end = clock();
        // double elapsed = (double(end-start))/ CLOCKS_PER_SEC;
        // printf("time taken by sws_scale in seconds : %f\n", elapsed);
        // cv::imwrite("sample.jpg", rgb_cpu);
        // continue;
        
        // output_frame = av_frame_alloc();

        //   if ((ret = av_hwframe_get_buffer(output_codec_ctx->hw_frames_ctx, output_frame, 0)) < 0) {
        //     fprintf(stderr, "Error code: %d.\n", (ret));
        //     return -1;
        // }
        

        // if ((ret = av_hwframe_transfer_data(output_frame, input_frame, 0)) < 0) {
        //     fprintf(stderr, "Error while transferring frame data to surface."
        //             "Error code: %d.\n", (ret));
        //     return 1;
        // }

        // output_frame->format = AV_PIX_FMT_RGB24;
        // output_frame->width = width;
        // output_frame->height = height;
        // ret = av_image_fill_arrays(output_frame->data, output_frame->linesize, oDeviceDest.data(),
        //                        AV_PIX_FMT_RGBA, width, height, 1);
 


        // output_frame->data[0] = oDeviceDest.data();
        // output_frame->linesize[0] = oDeviceDest.pitch();
     

        // if (i == -10){
        //     FILE *file = fopen("output.ppm", "wb");
        //     int num_bytes = av_image_get_buffer_size((AVPixelFormat)output_frame->format, output_frame->width, output_frame->height, 1);
        //     fprintf(file, "P6\n%d %d\n255\n", output_frame->width, output_frame->height);

        //     uint8_t *buffer = (u_int8_t *)av_malloc(num_bytes * sizeof(uint8_t));
        //     av_image_copy_to_buffer(buffer, num_bytes, (const uint8_t * const *)output_frame->data, (const int *)output_frame->linesize, (AVPixelFormat)output_frame->format, output_frame->width, output_frame->height, 1);
        //     fwrite(buffer, 1, num_bytes, file);

        //     fclose(file);


        // }
        // i++;
        // continue;

        // av_hwframe_transfer_data(gpu_frame, input_frame, 0);

        AVPacket *packet = av_packet_alloc();
        // // av_init_packet(&packet);
        // // packet.data = nullptr;
        // // packet.size = 0;
        clock_t start2 = clock();

        ret = avcodec_send_frame(output_codec_ctx, output_frame);
         if (ret < 0) 
        {
            fprintf(stderr, "Error sending a frame for encoding\n");
        }
        ret = avcodec_receive_packet(output_codec_ctx, packet);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
         

        }
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
        } else {
    
          
            // if (n_curr_cam_index) printf("timestamp:%I64d, %d\n", time_stamp, frame_num);
            // pkts_[n_curr_cam_index]->pts = av_rescale_q(time_stamp, {1, 1000}, video_streams_[n_curr_cam_index]->time_base);
            packet->pts =  i++; // Presentation timestamp

            av_packet_rescale_ts(packet, output_codec_ctx->time_base, output_stream->time_base);
            packet->dts = packet->pts;
            packet->stream_index = output_stream->index;

          

        }

        clock_t end2 = clock();
        //  elapsed = (double(end2-start2))/ CLOCKS_PER_SEC;
        // printf("time taken by  in seconds : %f\n", elapsed);
     
        if (ret == 0) {
            if (av_interleaved_write_frame(output_ctx, packet) < 0) {
                throw std::runtime_error("Failed to write frame to output file");
            }
            av_packet_unref(packet);
        } else if (ret != AVERROR(EAGAIN)) {
            throw std::runtime_error("Failed to receive packet from encoder");
        }
        // av_frame_free(&output_frame);
        // rgba_gpu.allocator->free(&rgba_gpu);

        // Encode and write the converted frame to the output file
        // (this part is not included in the snippet)
    }
    av_write_trailer(output_ctx);
    fclose(input_file);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);
    sws_freeContext(sws_ctx);
    avformat_free_context(output_ctx);
    avcodec_free_context(&output_codec_ctx);

    // std::cerr << std::endl << "Press enter to exit." << std::endl;
    // while (std::cin.get() != '\n');


    return 0;
}