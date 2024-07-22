extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/hwcontext.h>
}

#include <stdio.h>

static AVBufferRef *hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
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

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

// Function to initialize the CUDA hardware device
// static int init_hw_device(AVCodecContext *ctx, const enum AVHWDeviceType type) {
//     AVBufferRef *hw_device_ctx = NULL;
//     int err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0);
//     if (err < 0) {
//         fprintf(stderr, "Failed to create specified HW device.\n");
//         return err;
//     }
//     ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
//     av_buffer_unref(&hw_device_ctx);
//     return 0;
// }

int main(int argc, char *argv[]) {
    const char *output_filename = "output.mp4";
    const int width = 4096;
    const int height = 3000;
    const int fps = 9;

    AVFormatContext *output_format_context = NULL;
    AVStream *video_stream = NULL;
    AVCodecContext *codec_context = NULL;
    enum AVHWDeviceType type;
    type = av_hwdevice_find_type_by_name("cuda");

    avdevice_register_all();
    // avcodec_register_all();
    // av_register_all();

    // Allocate format context for the output
    avformat_alloc_output_context2(&output_format_context, NULL, NULL, output_filename);
    if (!output_format_context) {
        fprintf(stderr, "Could not create output context\n");
        return 1;
    }

    AVCodec *codec = avcodec_find_encoder_by_name("h264_nvenc");
    if (!codec) {
        fprintf(stderr, "Codec 'h264_nvenc' not found\n");
        return 1;
    }

    video_stream = avformat_new_stream(output_format_context, codec);
    if (!video_stream) {
        fprintf(stderr, "Could not create video stream\n");
        return 1;
    }

    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context\n");
        return 1;
    }

    codec_context->width = width;
    codec_context->height = height;
    codec_context->time_base = (AVRational){1, fps};
    codec_context->framerate = (AVRational){fps, 1};
    codec_context->pix_fmt = AV_PIX_FMT_CUDA;
    codec_context->get_format = get_hw_format;
    for (int i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    codec->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Initialize the CUDA hardware device
    if (hw_decoder_init(codec_context, AV_HWDEVICE_TYPE_CUDA) < 0) {
        fprintf(stderr, "Failed to initialize CUDA device\n");
        return 1;
    }

    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return 1;
    }

    video_stream->time_base = codec_context->time_base;
    avcodec_parameters_from_context(video_stream->codecpar, codec_context);

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_format_context->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open output file %s\n", output_filename);
            return 1;
        }
    }

    if (avformat_write_header(output_format_context, NULL) < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return 1;
    }

    // Assuming buffer is provided by the camera
    // Example buffer size for one frame in Bayer RGGB8 format
    int frame_size = width * height;
    uint8_t *buffer = (uint8_t*)av_malloc(frame_size);

    AVFrame *frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_CUDA;  // Set to CUDA format
    frame->width = width;
    frame->height = height;

    if (av_frame_get_buffer(frame, 0) < 0) {
        fprintf(stderr, "Could not allocate frame data\n");
        return 1;
    }

    int ret, i = 0;

    // Continuously process frames from the camera buffer
    while (1) {
        // Assuming buffer is filled with new data from the camera
        // In an actual implementation, this would be replaced by the camera's API
        // to get the latest frame. For example:
        // ret = get_camera_frame(buffer);
        // if (ret < 0) {
        //     fprintf(stderr, "Failed to get frame from camera\n");
        //     break;
        // }

        // Fill the GPU frame with raw data from the buffer
        AVFrame *sw_frame = av_frame_alloc();
        sw_frame->format = AV_PIX_FMT_BAYER_RGGB8;
        sw_frame->width = width;
        sw_frame->height = height;
        sw_frame->data[0] = buffer; // Directly use the camera buffer
        sw_frame->linesize[0] = width; // Assuming the stride is the same as the width

        // Upload the software frame to the GPU
        ret = av_hwframe_transfer_data(frame, sw_frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error transferring the frame to GPU\n");
            av_frame_free(&sw_frame);
            break;
        }

        frame->pts = i++;

        if ((ret = avcodec_send_frame(codec_context, frame)) < 0) {
            fprintf(stderr, "Error sending a frame for encoding\n");
            av_frame_free(&sw_frame);
            break;
        }

        AVPacket pkt = { 0 };
        av_init_packet(&pkt);

        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_context, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                fprintf(stderr, "Error during encoding\n");
                break;
            }

            pkt.stream_index = video_stream->index;
            av_packet_rescale_ts(&pkt, codec_context->time_base, video_stream->time_base);

            if (av_interleaved_write_frame(output_format_context, &pkt) < 0) {
                fprintf(stderr, "Error while writing video frame\n");
                break;
            }

            av_packet_unref(&pkt);
        }

        av_frame_free(&sw_frame);
    }

    av_write_trailer(output_format_context);

    // Cleanup
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    av_free(buffer);

    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_format_context->pb);
    }
    avformat_free_context(output_format_context);

    return 0;
}