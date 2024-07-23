// #include <gst/gst.h>
// #include <gst/app/gstappsrc.h>
// #include <iostream>
// #include <fstream>
// #include <vector>

// struct FrameData {
//     std::ifstream frame_file;
//     std::vector<guint8> frame_data;
//     gsize frame_size;
// };

// void need_data_callback(GstElement *appsrc, guint unused_size, gpointer user_data) {
//     static  guint64 timestamp = 0;
//     FrameData *frame_data_struct = static_cast<FrameData*>(user_data);
//     GstBuffer *buffer;
//     GstFlowReturn ret;
//     GstMapInfo map;

//     if (!frame_data_struct->frame_file.read(reinterpret_cast<char*>(frame_data_struct->frame_data.data()), frame_data_struct->frame_size)) {
//         // End-of-stream
//         g_signal_emit_by_name(appsrc, "end-of-stream", &ret);
//         return;
//     }

//     buffer = gst_buffer_new_allocate(NULL, frame_data_struct->frame_size, NULL);
//     gst_buffer_map(buffer, &map, GST_MAP_WRITE);
//     memcpy(map.data, frame_data_struct->frame_data.data(), frame_data_struct->frame_size);
//     gst_buffer_unmap(buffer, &map);

//     GST_BUFFER_PTS(buffer) = timestamp;
//     GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // Adjust the framerate
//     timestamp += GST_BUFFER_DURATION(buffer);

//     g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
//     gst_buffer_unref(buffer);

//     if (ret != GST_FLOW_OK) {
//         std::cerr << "Error pushing buffer to appsrc" << std::endl;
//     }
// }

// int main(int argc, char *argv[]) {
//     gst_init(&argc, &argv);

//     GstElement *pipeline = gst_pipeline_new("pipeline");
//     GstElement *appsrc = gst_element_factory_make("appsrc", "source");
//     GstElement *bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
//     // GstElement *videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
//      GstElement *videoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
//     // GstElement *x264enc = gst_element_factory_make("x264enc", "x264enc");
//     GstElement *x264enc =  gst_element_factory_make("nvv4l2h264enc", "encoder");
//     g_object_set(G_OBJECT( x264enc), "bitrate", 5000000, NULL);
//     GstElement *h264parse=  gst_element_factory_make("h264parse", "h264parse");
//     GstElement *mp4mux = gst_element_factory_make("mp4mux", "mp4mux");
//     GstElement *filesink = gst_element_factory_make("filesink", "filesink");

//     // nvvidconvs_[i] = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
//     //     encoders_[i] = gst_element_factory_make("nvv4l2h264enc", "encoder");
//     //     g_object_set(G_OBJECT( encoders_[i]), "bitrate", 5000000, NULL);
//     //     h264parse_[i] = gst_element_factory_make("h264parse", "h264parse");
//     //     sinks_[i] = gst_element_factory_make("filesink", "sink");
//         // std::string file_name = mapSerialNums_[i]  + "_test.mp4" ;

//     if (!pipeline || !appsrc || !bayer2rgb || !videoconvert || !x264enc || !h264parse || !mp4mux || !filesink) {
//         std::cerr << "Not all elements could be created." << std::endl;
//         return -1;
//     }

//     g_object_set(G_OBJECT(appsrc), "caps",
//                  gst_caps_new_simple("video/x-bayer",
//                                      "format", G_TYPE_STRING, "rggb",
//                                      "width", G_TYPE_INT, 4096,
//                                      "height", G_TYPE_INT, 3000,
//                                      "framerate", GST_TYPE_FRACTION, 30, 1,
//                                      NULL), NULL);

//     g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

//     gst_bin_add_many(GST_BIN(pipeline), appsrc, bayer2rgb, videoconvert, x264enc, h264parse, mp4mux, filesink, NULL);
//     if (!gst_element_link_many(appsrc, bayer2rgb, videoconvert, x264enc, h264parse, mp4mux, filesink, NULL)) {
//         std::cerr << "Elements could not be linked." << std::endl;
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Initialize frame data structure
//     FrameData frame_data_struct;
//     frame_data_struct.frame_file.open("/home/hamit/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", std::ios::binary);
//     if (!frame_data_struct.frame_file) {
//         std::cerr << "Could not open frame file." << std::endl;
//         return -1;
//     }

//     // Determine the frame size
//     // frame_data_struct.frame_file.seekg(0, std::ios::end);
//     // frame_data_struct.frame_size = frame_data_struct.frame_file.tellg();
//     // frame_data_struct.frame_file.seekg(0, std::ios::beg);
//     // frame_data_struct.frame_data.resize(frame_data_struct.frame_size);
//     frame_data_struct.frame_size = 4096 * 3000;
//     frame_data_struct.frame_data.resize(frame_data_struct.frame_size);

//     // Connect the need-data signal
//     g_signal_connect(appsrc, "need-data", G_CALLBACK(need_data_callback), &frame_data_struct);

//     GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
//     if (ret == GST_STATE_CHANGE_FAILURE) {
//         std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
//         gst_object_unref(pipeline);
//         return -1;
//     }

//     // Create and run the main loop
//     GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
//     GstBus *bus = gst_element_get_bus(pipeline);
//     gst_bus_add_watch(bus, [](GstBus *bus, GstMessage *msg, gpointer data) -> gboolean {
//         GMainLoop *loop = static_cast<GMainLoop*>(data);
//         switch (GST_MESSAGE_TYPE(msg)) {
//             case GST_MESSAGE_EOS:
//                 std::cout << "End of stream" << std::endl;
//                 g_main_loop_quit(loop);
//                 break;
//             case GST_MESSAGE_ERROR: {
//                 GError *err;
//                 gchar *debug_info;
//                 gst_message_parse_error(msg, &err, &debug_info);
//                 std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
//                 std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
//                 g_clear_error(&err);
//                 g_free(debug_info);
//                 g_main_loop_quit(loop);
//                 break;
//             }
//             default:
//                 break;
//         }
//         return TRUE;
//     }, main_loop);

//     g_main_loop_run(main_loop);

//     // Free resources
//     g_main_loop_unref(main_loop);
//     // gst_object_unref(bus);
//     gst_element_set_state(pipeline, GST_STATE_NULL);
//     gst_object_unref(pipeline);

//     return 0;
// }
#include <gst/gst.h>
#include <iostream>

void check_element_creation(GstElement *element, const char *name) {
    if (!element) {
        std::cerr << "Failed to create element: " << name << std::endl;
        exit(-1);
    }
}

void check_linking(bool result, const char *description) {
    if (!result) {
        std::cerr << "Failed to link elements: " << description << std::endl;
        exit(-1);
    }
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *filesrc, *bayer2rgb, *nvvideoconvert, *nvv4l2h264enc, *h264parse, *filesink;
    GstCaps *bayer_caps, *nvmm_caps;
    GstBus *bus;
    GstMessage *msg;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the elements
    filesrc = gst_element_factory_make("filesrc", "source");
    check_element_creation(filesrc, "filesrc");

    bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    check_element_creation(bayer2rgb, "bayer2rgb");

    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    check_element_creation(nvvideoconvert, "nvvideoconvert");

    nvv4l2h264enc = gst_element_factory_make("nvv4l2h264enc", "nvv4l2h264enc");
    check_element_creation(nvv4l2h264enc, "nvv4l2h264enc");

    h264parse = gst_element_factory_make("h264parse", "h264parse");
    check_element_creation(h264parse, "h264parse");

    filesink = gst_element_factory_make("filesink", "sink");
    check_element_creation(filesink, "filesink");

    // Create the empty pipeline
    pipeline = gst_pipeline_new("video-pipeline");
    if (!pipeline) {
        std::cerr << "Failed to create pipeline." << std::endl;
        return -1;
    }

    // Set the element properties
    g_object_set(G_OBJECT(filesrc), "location", "bayer8.bin", "blocksize", 12288000, NULL);
    g_object_set(G_OBJECT(nvv4l2h264enc), "bitrate", 5000000, NULL);
    g_object_set(G_OBJECT(filesink), "location", "./records/camera1.mp4", NULL);

    // Create the caps
    bayer_caps = gst_caps_new_simple("video/x-bayer",
                                     "width", G_TYPE_INT, 4096,
                                     "height", G_TYPE_INT, 3000,
                                     "framerate", GST_TYPE_FRACTION, 9, 1,
                                     "format", G_TYPE_STRING, "rggb",
                                     NULL);

    nvmm_caps = gst_caps_new_simple("video/x-raw",
                                    "format", G_TYPE_STRING, "NV12",
                                    "memory", G_TYPE_STRING, "NVMM",
                                    NULL);

    // Build the pipeline
    gst_bin_add_many(GST_BIN(pipeline), filesrc, bayer2rgb, nvvideoconvert, nvv4l2h264enc, h264parse, filesink, NULL);

    check_linking(gst_element_link_filtered(filesrc, bayer2rgb, bayer_caps), "filesrc -> bayer2rgb with caps");
    gst_caps_unref(bayer_caps);

    check_linking(gst_element_link_filtered(bayer2rgb, nvvideoconvert, nvmm_caps), "bayer2rgb -> nvvideoconvert with caps");
    gst_caps_unref(nvmm_caps);

    check_linking(gst_element_link_many(nvvideoconvert, nvv4l2h264enc, h264parse, filesink, NULL), "nvvideoconvert -> nvv4l2h264enc -> h264parse -> filesink");

    // Start playing
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until error or EOS
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Parse message
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
                std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                std::cout << "End-Of-Stream reached." << std::endl;
                break;
            default:
                std::cerr << "Unexpected message received." << std::endl;
                break;
        }
        gst_message_unref(msg);
    }

    // Free resources
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}