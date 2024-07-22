#include <gst/gst.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdbool.h>
#include <vector>
#include <arv.h>
#include <iostream>
#include <thread>
#include <functional> 
#include <signal.h>

std::vector<GMainLoop *> main_loop;
std::vector<GstElement *> pipeline;
std::vector<GstBus *> bus;
gchar *fps_msg;
unsigned int n_devices;
void ctrlC (int)
{
  
  printf ("\nCtrl-C detected, exit condition set to true.\n");
  for (int i = 0; i<n_devices; i++)
    gst_element_send_event(pipeline[i], gst_event_new_eos());

  // BaslerMultipleCameras::m_waitObject.Signal();


}

static void fps_measurements_callback(GstElement *fpsdisplaysink, gdouble fps, gdouble droprate, gdouble avgfps, gpointer user_data) {
    g_print("FPS: %.2f, Drop Rate: %.2f, Average FPS: %.2f\n", fps, droprate, avgfps);
}

// Function to handle messages from the GStreamer bus
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
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
void start_pipeline(  unsigned int n_cam_index){
      // // Start the pipeline
    int ret = gst_element_set_state(pipeline[n_cam_index], GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipeline[n_cam_index]);
        return ;
    }

    main_loop[n_cam_index] = g_main_loop_new(NULL, FALSE);

    // Add a bus watch to handle messages from the pipeline
    bus[n_cam_index] = gst_element_get_bus(pipeline[n_cam_index]);
    gst_bus_add_watch(bus[n_cam_index], bus_call, main_loop[n_cam_index]);
    gst_object_unref(bus[n_cam_index]);
    g_main_loop_run(main_loop[n_cam_index]);
    // // Stop the pipeline

    gst_element_set_state(pipeline[n_cam_index], GST_STATE_NULL);

    // // Clean up
    gst_object_unref(pipeline[n_cam_index]);
    g_main_loop_unref(main_loop[n_cam_index]);

}

int main(int argc, char *argv[]) {
    std::vector<GstElement *>  aravissrc, fps, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink;
    std::vector<std::string> devices_id;
    std::vector<GstCaps *> caps;
    GstStateChangeReturn ret;
   
    GtkListStore *list_store;
	GtkTreeIter iter;
    std::vector<std::thread> th_vec;
    // Initialize GStreamer
    gst_init(&argc, &argv);
    // gst_debug_set_default_threshold(GST_LEVEL_INFO);
    signal (SIGINT, ctrlC);

    arv_update_device_list();
    n_devices = 6; //arv_get_n_devices ();
    std::cout<<"number of devices:"<<n_devices<<std::endl;
    // arv_camera_new
    for (int i = 0; i < n_devices; i++) {
		GString *protocol;

        devices_id.push_back(arv_get_device_id(i));
        printf("%s\n", devices_id.back().c_str());
		// g_string_ascii_down (protocol);

		// gtk_list_store_append (list_store, &iter);
		// gtk_list_store_set (list_store, &iter,
		// 		    0, arv_get_device_id (i),
		// 		    1, protocol->str,
		// 		    2, arv_get_device_vendor (i),
		// 		    3, arv_get_device_model (i),
		// 		    4, arv_get_device_serial_nbr (i),
		// 		    -1);
        std::string pipeline_name = "my-pipeline_" + std::to_string(i);
        pipeline.push_back(gst_pipeline_new(pipeline_name.c_str()));
        aravissrc.push_back(gst_element_factory_make("aravissrc", std::string("source_" + std::to_string(i)).c_str()));
       
        caps.push_back( gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb"));
        bayer2rgb.push_back(gst_element_factory_make("bayer2rgb", std::string("bayer2rgb_" + std::to_string(i)).c_str()));
        nvvideoconvert.push_back(gst_element_factory_make("nvvideoconvert",  std::string("nvvideoconvert_" + std::to_string(i)).c_str()));
        nvh264enc.push_back( gst_element_factory_make("nvv4l2h264enc", std::string("encoder_" + std::to_string(i)).c_str()));
        g_object_set(G_OBJECT(nvh264enc.back()), "bitrate", 5000000, NULL);
        h264parse.push_back(gst_element_factory_make("h264parse", std::string("h264parse_" + std::to_string(i)).c_str()));
        filesink.push_back( gst_element_factory_make("filesink", std::string("sink_" + std::to_string(i)).c_str()));
        char file_name[256];
        sprintf(file_name, "%s.mp4", arv_get_device_serial_nbr(i));
        g_object_set(G_OBJECT(filesink.back()), "location", file_name , NULL);

        fps.push_back(gst_element_factory_make("fpsdisplaysink", std::string("fps" + std::to_string(i)).c_str()));
        g_object_set (G_OBJECT (fps.back()), "text-overlay", FALSE, "video-sink", filesink.back(), NULL);
        g_object_set(fps.back(), "sync", FALSE, NULL);
        g_object_set(fps.back(), "signal-fps-measurements", TRUE, NULL);
        g_signal_connect(fps.back(), "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);

        //  g_object_set(G_OBJECT(fps.back()),
        // "video-sink", "autovideosink",
        // // "text-overlay", false,
        // NULL
        //  );
        // g_object_get (G_OBJECT (fps.back()), "last-message", &fps_msg, NULL);
        // delay_show_FPS++;
        // if (fps_msg != NULL) {
        //     if ((delay_show_FPS % DELAY_VALUE) == 0) {
        //     g_print ("Frame info: %s\n", fps_msg);
        //     delay_show_FPS = 0;
        //     }
        // }

        gst_bin_add_many(GST_BIN(pipeline.back()), aravissrc.back(),  fps.back(), bayer2rgb.back(), nvvideoconvert.back(),  nvh264enc.back(), h264parse.back(), filesink.back(), NULL);
        gst_element_link_filtered(aravissrc.back(), bayer2rgb.back(), caps.back());
        gst_caps_unref(caps.back());
        gst_element_link_many(bayer2rgb.back(), nvvideoconvert.back(), nvh264enc.back(), h264parse.back(), filesink.back(), NULL);
        g_object_set(G_OBJECT(aravissrc.back()), 
            "camera-name", devices_id.back().c_str(),
            "gain", 20.0,
            "exposure", 5000.0,
            "packet-size", 1500,           
            "packet-delay", 1024,          
             NULL);
        main_loop.push_back(nullptr);
        bus.push_back(nullptr);
	}

    for (unsigned int i = 0; i < n_devices; i++)
    {

        th_vec.emplace_back(std::thread(std::bind(start_pipeline,  i)));
              
    }

    for (auto &th: th_vec)
        th.join();
    



    // // Create the pipeline
    
    // pipeline = gst_pipeline_new("my-pipeline");

    // // Create the elements
    // aravissrc = gst_element_factory_make("aravissrc", "source");
    // caps = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb");
    // bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    // nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    // nvh264enc = gst_element_factory_make("nvv4l2h264enc", "encoder");
    // g_object_set(G_OBJECT(nvh264enc), "bitrate", 5000000, NULL);
    // h264parse = gst_element_factory_make("h264parse", "h264parse");
    // filesink = gst_element_factory_make("filesink", "sink");
    // g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

    // // Add the elements to the pipeline
    // gst_bin_add_many(GST_BIN(pipeline), aravissrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // // Link the elements
    // gst_element_link_filtered(aravissrc, bayer2rgb, caps);
    // gst_caps_unref(caps);
    // gst_element_link_many(bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // // Configure aravissrc
    // g_object_set(G_OBJECT(aravissrc), 
    //            //  "camera-name", "Basler Camera", // Adjust the camera name if needed
    //              "gain", 20.0,
    //              "exposure", 5000.0,
    //              "packet-size", 1500,            // Set the packet size (example value)
    //              "packet-delay", 1000,           // Set the packet delay (example value)
               
    //              NULL);

    // // Start the pipeline
    // ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // if (ret == GST_STATE_CHANGE_FAILURE) {
    //     g_printerr("Failed to start the pipeline.\n");
    //     gst_object_unref(pipeline);
    //     return -1;
    // }

    // // Create a GLib Main Loop and set it to run
    // main_loop = g_main_loop_new(NULL, FALSE);

    // // Add a bus watch to handle messages from the pipeline
    // bus = gst_element_get_bus(pipeline);
    // gst_bus_add_watch(bus, bus_call, main_loop);
    // gst_object_unref(bus);

    // // Run the main loop
    // g_main_loop_run(main_loop);

    // // Stop the pipeline
    // gst_element_set_state(pipeline, GST_STATE_NULL);

    // // Clean up
    // gst_object_unref(pipeline);
    // g_main_loop_unref(main_loop);

    return 0;
}