#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

// static AVBufferRef *hw_device_ctx = NULL;
// static enum AVPixelFormat hw_pix_fmt;
// static FILE *output_file = NULL;

// static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
// {
//     int err = 0;

//     if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
//                                       NULL, NULL, 0)) < 0) {
//         fprintf(stderr, "Failed to create specified HW device.\n");
//         return err;
//     }
//     ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

//     return err;
// }

// static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
//                                         const enum AVPixelFormat *pix_fmts)
// {
//     const enum AVPixelFormat *p;

//     for (p = pix_fmts; *p != -1; p++) {
//         if (*p == hw_pix_fmt)
//             return *p;
//     }

//     fprintf(stderr, "Failed to get HW surface format.\n");
//     return AV_PIX_FMT_NONE;
// }

// static int decode_write(AVCodecContext *avctx, AVFrame *frame)
// {
//     AVFrame *sw_frame = NULL;
//     int ret = 0;

//     if (frame->format == hw_pix_fmt) {
//         /* retrieve data from GPU to CPU */
//         if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
//             fprintf(stderr, "Error transferring the data to system memory\n");
//             return ret;
//         }
//         frame = sw_frame;
//     }

//     int size = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width,
//                                         frame->height, 1);
//     uint8_t *buffer = (uint8_t*)av_malloc(size);
//     if (!buffer) {
//         fprintf(stderr, "Can not alloc buffer\n");
//         return AVERROR(ENOMEM);
//     }
//     ret = av_image_copy_to_buffer(buffer, size,
//                                   (const uint8_t * const *)frame->data,
//                                   (const int *)frame->linesize, (AVPixelFormat)frame->format,
//                                   frame->width, frame->height, 1);
//     if (ret < 0) {
//         fprintf(stderr, "Can not copy image to buffer\n");
//         av_freep(&buffer);
//         return ret;
//     }

//     if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
//         fprintf(stderr, "Failed to dump raw data.\n");
//         av_freep(&buffer);
//         return ret;
//     }

//     av_frame_free(&sw_frame);
//     av_freep(&buffer);
//     return 0;
// }

// int main(int argc, char *argv[])
// {
//     AVCodecContext *decoder_ctx = NULL;
//     AVCodec *decoder = NULL;
//     AVFrame *frame = NULL;
//     enum AVHWDeviceType type;
//     int i, ret;
//     int width = 4096, height = 3000; // Update these values as per your raw file
//     int frame_size = width * height * 1; // Assuming BAYER_RGGB8 format
//     FILE *input_file = NULL;
//     uint8_t *frame_data = (uint8_t*)av_malloc(frame_size);

//     // if (argc < 3) {
//     //     fprintf(stderr, "Usage: %s <device type> <input file> <output file>\n", argv[0]);
//     //     return -1;
//     // }
//       /* find the decoder */
//     decoder = avcodec_find_decoder(AV_CODEC_ID_RAWVIDEO);
//     if (!decoder) {
//         fprintf(stderr, "Failed to find RAW video decoder\n");
//         return -1;
//     }

//     type = av_hwdevice_find_type_by_name("cuda");
//     if (type == AV_HWDEVICE_TYPE_NONE) {
//         fprintf(stderr, "Device type %s is not supported.\n", "cuda");
//         fprintf(stderr, "Available device types:");
//         while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
//             fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
//         fprintf(stderr, "\n");
//         return -1;
//     }


//      for (i = 0;; i++) {
//         const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
//         if (!config) {
//             fprintf(stderr, "Decoder %s does not support device type %s.\n",
//                     decoder->name, av_hwdevice_get_type_name(type));
//             return -1;
//         }
//         if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
//             config->device_type == type) {
//             hw_pix_fmt = config->pix_fmt;
//             break;
//         }
//     }
//     /* open the input file */
//     input_file = fopen("/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", "rb");
//     if (!input_file) {
//         fprintf(stderr, "Cannot open input file '%s'\n", argv[2]);
//         return -1;
//     }

//     /* find the decoder */
//     // decoder = avcodec_find_decoder(AV_CODEC_ID_RAWVIDEO);
//     // if (!decoder) {
//     //     fprintf(stderr, "Failed to find RAW video decoder\n");
//     //     return -1;
//     // }

//     if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
//         return AVERROR(ENOMEM);

//     decoder_ctx->width = width;
//     decoder_ctx->height = height;
//     decoder_ctx->pix_fmt = AV_PIX_FMT_BAYER_RGGB8;
//     decoder_ctx->get_format = get_hw_format;




//     if (hw_decoder_init(decoder_ctx, type) < 0)
//         return -1;

//     if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
//         fprintf(stderr, "Failed to open codec\n");
//         return -1;
//     }

//     /* open the file to dump raw data */
//     output_file = fopen("/home/spaceport/Softwares/GrabberBaslerMultipleCameras/build/output3.bin", "w");

//     /* actual decoding and dump the raw data */
   
//     // int ret, i = 0;

//     while (fread(frame_data, 1, frame_size, input_file) == frame_size) {
//     // for (i = 0; i < 200; i++) {
//         // if (fread(frame_data, 1, frame_size, input_file) != frame_size) {
//         //     fprintf(stderr, "Error reading frame data from input file\n");
//         //     break;
//         // }

//         if (!(frame = av_frame_alloc())) {
//             fprintf(stderr, "Can not alloc frame\n");
//             break;
//         }

//         frame->format = decoder_ctx->pix_fmt;
//         frame->width = decoder_ctx->width;
//         frame->height = decoder_ctx->height;

//         if (av_image_fill_arrays(frame->data, frame->linesize, frame_data,
//                                  (AVPixelFormat)frame->format, frame->width, frame->height, 1) < 0) {
//             fprintf(stderr, "Error filling frame data\n");
//             av_frame_free(&frame);
//             break;
//         }

//         ret = decode_write(decoder_ctx, frame);
//         av_frame_free(&frame);
//         if (ret < 0)
//             break;
//     }

//     if (output_file)
//         fclose(output_file);
//     avcodec_free_context(&decoder_ctx);
//     fclose(input_file);
//     av_buffer_unref(&hw_device_ctx);
//     av_freep(&frame_data);

//     return 0;
// }


/*
 * Copyright (c) 2001 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * video encoding with libavcodec API example
 *
 * @example encode_video.c
 */


static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }

        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

int main(int argc, char **argv)
{
    const char *filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    if (argc <= 2) {
        fprintf(stderr, "Usage: %s <output file> <codec name>\n", argv[0]);
        exit(0);
    }
    filename = argv[1];
    codec_name = argv[2];

    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }

    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);

    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %d\n", (ret));
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;

    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

    /* encode 1 second of video */
    for (i = 0; i < 25; i++) {
        fflush(stdout);

        /* make sure the frame data is writable */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);

        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }

        /* Cb and Cr */
        for (y = 0; y < c->height/2; y++) {
            for (x = 0; x < c->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }

        frame->pts = i;

        /* encode the image */
        encode(c, frame, pkt, f);
    }

    /* flush the encoder */
    encode(c, NULL, pkt, f);

    /* add sequence end code to have a real MPEG file */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
        fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}