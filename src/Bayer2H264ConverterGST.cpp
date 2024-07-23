#include "Bayer2H264ConverterGST.h"

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


// struct DATA;

BayerToH264ConverterGST::BayerToH264ConverterGST(std::map<int, std::string> mapSerialNums, unsigned int input_width, unsigned int input_height):
    width_(input_width), 
    height_(input_height),
    size_(input_width*input_height),
    mapSerialNums_(mapSerialNums)

        
    {
            
        num_devices_ = mapSerialNums_.size();
        pipelines_.resize(num_devices_, nullptr);
        sources_.resize(num_devices_, nullptr);
        capsfilters_.resize(num_devices_, nullptr);
        capsfilters2_.resize(num_devices_, nullptr);
        bayer_to_rgb_.resize(num_devices_, nullptr);
        caps_.resize(num_devices_, nullptr);
        caps2_.resize(num_devices_, nullptr);

        vidconvs_.resize(num_devices_, nullptr);
        nvvidconvs_.resize(num_devices_, nullptr);
        encoders_.resize(num_devices_, nullptr);
        h264parse_.resize(num_devices_, nullptr);
        muxers_.resize(num_devices_, nullptr);
        sinks_.resize(num_devices_, nullptr);
        buses_.resize(num_devices_, nullptr);
        msgs_.resize(num_devices_, nullptr);
        main_loops_.resize(num_devices_, nullptr);
        rets_.resize(num_devices_);
           
        // datas_.resize(num_devices_);
        results_.resize(num_devices_, false);
        frame_cnts.resize(num_devices_, 0);
        // caps_ = nullptr;
        // codecMutexes_ = std::vector<std::mutex>(num_devices_);

        // InitializeGstPipeline();

    }
  
bool BayerToH264ConverterGST::CloseAllGstPipelines() {
    for (unsigned int i = 0; i < num_devices_; i++) {
        int flow_ret;
        // g_signal_emit_by_name (sources_[i], "end-of-stream", &flow_ret);
        gst_element_send_event(pipelines_[i], gst_event_new_eos());
        // g_main_loop_run(main_loops_[i]);
        gst_element_set_state(pipelines_[i], GST_STATE_NULL);
        gst_object_unref(buses_[i]);
        // Clean up
        gst_object_unref(pipelines_[i]);
        // g_main_loop_quit(main_loop);
        g_main_loop_unref(main_loops_[i]);


    }
    
    return true;

}
    // void BayerToH264ConverterGST::start_feed (GstElement * pipeline, guint size, DATA *data_struct)
    // {
        
   
 
    //     if (data_struct->source_id == 0) {
    //          g_print ("start feeding\n");
    //         data_struct->source_id =  g_idle_add ((GSourceFunc) push_buffer, data_struct);
    //     }
    // }

  void BayerToH264ConverterGST::on_need_callback(GstElement *appsrc, guint unused_size,  DATA *data_struct) {


    if (BayerToH264ConverterGST::m_bExit == true)
        gst_element_send_event(data_struct->pipeline, gst_event_new_eos());

    static  guint64 timestamp = 0;
    const int DefaultTimeout_ms = 5000;
    
    CBaslerUniversalGrabResultPtr ptrGrabResult;
    data_struct->cam->RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );
    intptr_t cameraIndex = ptrGrabResult->GetCameraContext();
    if (ptrGrabResult->GrabSucceeded())
    {
        
        // if (i %10 == 0 /*&& (nCurCameraIndex ==2 || nCurCameraIndex ==1) */) 
        //     std::cout<<nCurCameraIndex<<". Cam, Timestamp:"<<std::fixed<< std::setprecision(6)<<double(ptrGrabResult->GetTimeStamp())/1.e9<<" s"<<std::endl;
        
        uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
        size_t bufferSize = ptrGrabResult->GetBufferSize();
        u_int64_t timeStamp = ptrGrabResult->GetTimeStamp();
        const std::string serialNumber{ data_struct->cam->GetDeviceInfo().GetSerialNumber().c_str()};
    

        GstBuffer *buffer = nullptr;
        GstFlowReturn ret;
        GstMapInfo map;
        // DATA *data_struct = static_cast<DATA*>(user_data);
        size_t size_ = bufferSize ; //data_struct->imageSize;
        // std::cout<<"camera size:"<<bufferSize<<std::endl;
        // guint8* camera_buffer = data_struct->image;

        // Allocate a new buffer
        buffer = gst_buffer_new_allocate(NULL, size_, NULL);
    
        // Map the buffer and fill it with data from your camera SDK
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);
        // Here you should copy your camera SDK buffer into map.data
        memcpy(map.data, pImageBuffer, size_);
        // memset(map.data, 0xff, size); // Dummy data for example
        gst_buffer_unmap(buffer, &map);
        
        // buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, camera_buffer, size_, 0, size_, nullptr, nullptr);

        GST_BUFFER_PTS(buffer) = timestamp;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // Adjust the framerate
        timestamp += GST_BUFFER_DURATION(buffer);

        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unref(buffer);

        if (ret != GST_FLOW_OK) {
            std::cerr << "Error pushing buffer to appsrc" << std::endl;
        }
    } else
    {
        std::cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << std::endl;//" " << ptrGrabResult->GetErrorDescription() << std::endl;
    }
    FPS_CALC("Grabbing Buffer FPS",  data_struct->cam_index);
     ptrGrabResult.Release();


}

bool BayerToH264ConverterGST::CloseSingleGstPipeline(unsigned int n_cam_index) {

    
    // gst_element_send_event(pipelines_[n_cam_index], gst_event_new_eos());
    g_main_loop_run(main_loops_[n_cam_index]);
    
    g_main_loop_unref(main_loops_[n_cam_index]);

    gst_object_unref(buses_[n_cam_index]);

    gst_element_set_state(pipelines_[n_cam_index], GST_STATE_NULL);
    // Clean up
    gst_object_unref(pipelines_[n_cam_index]);
    // g_main_loop_quit(main_loop);
    return true;

}

BayerToH264ConverterGST::~BayerToH264ConverterGST() 
{
//    close();
}

void BayerToH264ConverterGST::InitializeSingleGstPipeline(unsigned int n_cam_index, DATA *data_struct){

    pipelines_[n_cam_index] = gst_pipeline_new("bayer-to-h264");
    sources_[n_cam_index] = gst_element_factory_make("appsrc", "source");
    capsfilters_[n_cam_index] = gst_element_factory_make("capsfilter", "capsfilter");
    // caps_[n_cam_index] = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb");
    caps_[n_cam_index] = gst_caps_new_simple("video/x-bayer",
                            "format", G_TYPE_STRING, "rggb",
                            "width", G_TYPE_INT, 4096,
                            "height", G_TYPE_INT, 3000,
                            "framerate", GST_TYPE_FRACTION, 30, 1,
                            NULL);
    g_object_set(capsfilters_[n_cam_index], "caps", caps_[n_cam_index], NULL);
    gst_caps_unref(caps_[n_cam_index]);
    bayer_to_rgb_[n_cam_index] = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    // bayer_to_rgb_[n_cam_index] =  gst_element_factory_make("tcamconvert", "tcamconvert");
    // vidconvs_[n_cam_index] = gst_element_factory_make("videoconvert", "videoconvert");

    nvvidconvs_[n_cam_index] = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    encoders_[n_cam_index] = gst_element_factory_make("nvv4l2h264enc", "encoder");
    g_object_set(G_OBJECT( encoders_[n_cam_index]), "bitrate", 15000000, NULL);
    // g_object_set(G_OBJECT( encoders_[n_cam_index]),"stream-format", "byte-stream", NULL);

    capsfilters2_[n_cam_index] = gst_element_factory_make("capsfilter", "capsfilter2");

    caps_[n_cam_index] = gst_caps_new_simple("video/x-h264",
                                "streamformat", G_TYPE_STRING, "byte-stream",
                                NULL);
    g_object_set(capsfilters2_[n_cam_index], "caps", caps_[n_cam_index], NULL);                            
    gst_caps_unref(caps_[n_cam_index]); 

    h264parse_[n_cam_index] = gst_element_factory_make("h264parse", "h264parse");
    muxers_[n_cam_index] = gst_element_factory_make("qtmux", "muxer");
    sinks_[n_cam_index] = gst_element_factory_make("filesink", "sink");
    std::string file_name = mapSerialNums_[n_cam_index]  + "_test_h264.mp4" ;
    g_object_set(G_OBJECT(sinks_[n_cam_index]), "location", file_name.c_str(), NULL);

    // gst_element_link_pads (sources_[n_cam_index], "src", sinks_[n_cam_index], "sink");

   
     // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipelines_[n_cam_index]), sources_[n_cam_index], capsfilters_[n_cam_index], bayer_to_rgb_ [n_cam_index], nvvidconvs_[n_cam_index], encoders_[n_cam_index], capsfilters2_[n_cam_index],  h264parse_[n_cam_index], muxers_[n_cam_index], sinks_[n_cam_index], NULL);


    // gst_element_link_filtered( sources_[n_cam_index], bayer_to_rgb_[n_cam_index], caps_[n_cam_index]);

    // Link the elements
    gst_element_link_many(sources_[n_cam_index], capsfilters_[n_cam_index], bayer_to_rgb_[n_cam_index],  nvvidconvs_[n_cam_index], encoders_[n_cam_index], capsfilters2_[n_cam_index], h264parse_[n_cam_index], muxers_[n_cam_index], sinks_[n_cam_index], NULL);


    // Configure appsrc
    // g_object_set(G_OBJECT(sources_[n_cam_index]), "caps", caps_[n_cam_index], "format", GST_FORMAT_TIME, NULL);
    data_struct->appsrc = sources_[n_cam_index];
    data_struct->pipeline = pipelines_[n_cam_index];
    data_struct->cam_index = n_cam_index;
    g_signal_connect(sources_[n_cam_index], "need-data", G_CALLBACK(&BayerToH264ConverterGST::on_need_callback), data_struct);

            // Start the pipeline
    int ret = gst_element_set_state(pipelines_[n_cam_index], GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipelines_[n_cam_index]);
        return ;
    }


    main_loops_[n_cam_index] = g_main_loop_new(NULL, FALSE);
    buses_[n_cam_index] = gst_element_get_bus(pipelines_[n_cam_index]);
    // gst_bus_add_watch(buses_[n_cam_index], BayerToH264ConverterGST::bus_call,  main_loops_[n_cam_index]);

    gst_bus_add_watch(buses_[n_cam_index], [](GstBus *bus, GstMessage *msg, gpointer data) -> gboolean {
        GMainLoop *loop = static_cast<GMainLoop*>(data);
        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_EOS:
                std::cout << "End of stream" << std::endl;
                g_main_loop_quit(loop);
                break;
            case GST_MESSAGE_ERROR: {
                GError *err;
                gchar *debug_info;
                gst_message_parse_error(msg, &err, &debug_info);
                std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
                std::cerr << "Debugging information: " << (debug_info ? debug_info : "none") << std::endl;
                g_clear_error(&err);
                g_free(debug_info);
                g_main_loop_quit(loop);
                break;
            }
            default:
                break;
        }
        return TRUE;
    }, main_loops_[n_cam_index]);

    // gst_object_unref(buses_[n_cam_index]);

    

}

bool BayerToH264ConverterGST::StartSingleGstPipeline(unsigned int n_cam_index) {
    int ret = gst_element_set_state(pipelines_[n_cam_index], GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipelines_[n_cam_index]);
        return false;
    }
    // main_loops_[n_cam_index] = g_main_loop_new(NULL, FALSE);
    buses_[n_cam_index] = gst_element_get_bus(pipelines_[n_cam_index]);
    // gst_bus_add_watch(buses_[n_cam_index], BayerToH264ConverterGST::bus_call,  main_loops_[n_cam_index]);
    gst_object_unref(buses_[n_cam_index]);
    return true;
}
  

void BayerToH264ConverterGST::InitializeAllGstPipelines(){

    // caps_ = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb");
        
    for (unsigned int i = 0; i < num_devices_;i++){

        // Create elements
        pipelines_[i] = gst_pipeline_new("bayer-to-h264");
        sources_[i] = gst_element_factory_make("appsrc", "source");
        caps_[i] = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb");
        bayer_to_rgb_[i] = gst_element_factory_make("bayer2rgb", "bayer2rgb");
        nvvidconvs_[i] = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
        encoders_[i] = gst_element_factory_make("nvv4l2h264enc", "encoder");
        g_object_set(G_OBJECT( encoders_[i]), "bitrate", 5000000, NULL);
        h264parse_[i] = gst_element_factory_make("h264parse", "h264parse");
        sinks_[i] = gst_element_factory_make("filesink", "sink");
        std::string file_name = mapSerialNums_[i]  + "_test.mp4" ;
        g_object_set(G_OBJECT(sinks_[i]), "location", file_name.c_str(), NULL);

        // Add elements to the pipeline
        gst_bin_add_many(GST_BIN(pipelines_[i]), sources_[i], bayer_to_rgb_ [i], nvvidconvs_[i], encoders_[i], h264parse_[i],  sinks_[i], NULL);

        gst_element_link_filtered( sources_[i], bayer_to_rgb_[i], caps_[i]);
        gst_caps_unref(caps_[i]);

        // Link the elements
        gst_element_link_many(bayer_to_rgb_[i], nvvidconvs_[i], encoders_[i], h264parse_[i], sinks_[i], NULL);


        // Configure appsrc
        g_object_set(G_OBJECT(sources_[i]), "caps", caps_[i], "format", GST_FORMAT_TIME, NULL);
                // Start the pipeline
        int ret = gst_element_set_state(pipelines_[i], GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            g_printerr("Failed to start the pipeline.\n");
            gst_object_unref(pipelines_[i]);
            return ;
        }
        main_loops_[i] = g_main_loop_new(NULL, FALSE);
        buses_[i] = gst_element_get_bus(pipelines_[i]);
        gst_bus_add_watch(buses_[i], BayerToH264ConverterGST::bus_call,  main_loops_[i]);
        gst_object_unref(buses_[i]);


        // g_signal_connect(sources_[i], "need-data", G_CALLBACK(BayerToH264ConverterGST::on_need_data), datas_[i]);
    }
    // gst_caps_unref(caps_);

}

 gboolean BayerToH264ConverterGST::push_data2( guint8 *camera_buffer, unsigned int n_cam_index) {
    static thread_local guint64 timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;
    // guint size = 12288000; // Adjust the size according to your buffer size

      // buffer = gst_buffer_new_wrapped((gpointer)data->image.get(), CAMERA_BUFFER_SIZE);
    // buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, camera_buffer, size_, 0, size_, camera_buffer, (GDestroyNotify)free);



    GstMapInfo map;

    // Allocate a new buffer
    buffer = gst_buffer_new_allocate(NULL, size_, NULL);
  
    // Map the buffer and fill it with data from your camera SDK
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    // Here you should copy your camera SDK buffer into map.data
    memcpy(map.data, camera_buffer, size_);
    // memset(map.data, 0xff, size); // Dummy data for example
    gst_buffer_unmap(buffer, &map);

    // Set the buffer timestamp
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // Adjust the framerate
    timestamp += GST_BUFFER_DURATION(buffer);

    // Push the buffer into the appsrc
    g_signal_emit_by_name(sources_[n_cam_index], "push-buffer", buffer, &ret);

    // Free the buffer
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        // We got some error, stop sending data
        return FALSE;
    }

    return TRUE;
}

// Function to push data into appsrc
 gboolean BayerToH264ConverterGST::push_data( guint8 *camera_sdk_buffer, unsigned int n_cam_index) {
    static thread_local GstClockTime timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;

    // Wrap the camera SDK buffer in a GstBuffer
    buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, camera_sdk_buffer, size_, 0, size_, nullptr, nullptr);
    
    // buffer = gst_buffer_new_wrapped_full(static_cast<GstMemoryFlags>(GST_MEMORY_FLAG_READONLY | GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS), 
    //             camera_sdk_buffer, size_, 0, size_, nullptr, nullptr );
    // buffer = gst_buffer_new_wrapped(camera_sdk_buffer, size_);

    // Set the buffer timestamp
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 30); // Adjust the framerate
    timestamp += GST_BUFFER_DURATION(buffer);

    // Push the buffer into the appsrc
    g_signal_emit_by_name(sources_[n_cam_index], "push-buffer", buffer, &ret);
    // gst_app_src_push_buffer((GstAppSrc*)sources_[n_cam_index], buffer);
    // Unreference the buffer
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        // We got some error, stop sending data
        return FALSE;
    }

    return TRUE;
}
 gboolean BayerToH264ConverterGST::bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}



// static GstBuffer* receive_camera_buffer( DATA *data) {
//     GstBuffer *buffer = NULL;
//     static GstClockTime last_time = 0;
//     GstClockTime current_time = gst_util_get_timestamp();

//     // Check if it's time to generate a new frame
//     if (current_time - last_time >= frameDurationInMS * GST_MSECOND) {
//         last_time = current_time;

//         // Increment the buffer index (for simulating different frames)
//         // buffer_index = (buffer_index + 1) % 256;
//         // std::memset(camera_buffer, buffer_index, CAMERA_BUFFER_SIZE);

//         // Create a new GStreamer buffer that wraps the camera data
//         buffer = gst_buffer_new_wrapped((gpointer)data->image.get(), CAMERA_BUFFER_SIZE);
//     }

//     return buffer;
// }
  
// bool BayerToH264ConverterGST::convertAndEncodeBayerToH264( uint8_t *bayerData, unsigned int n_curr_cam_index,  int64_t time_stamp) 
// {
    

// }

   
  

   







