#include <gst/gst.h>
#include <string.h>
#include <stdbool.h>

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

int main(int argc, char *argv[]) {
    GstElement *pipeline, *aravissrc, *bayer2rgb, *nvvideoconvert, *nvh264enc, *h264parse, *filesink;
    GstCaps *caps;
    GstBus *bus;
    GstStateChangeReturn ret;
    GMainLoop *main_loop;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    pipeline = gst_pipeline_new("my-pipeline");

    // Create the elements
    aravissrc = gst_element_factory_make("aravissrc", "source");
    caps = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=30/1,format=rggb");
    bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    nvh264enc = gst_element_factory_make("nvv4l2h264enc", "encoder");
    g_object_set(G_OBJECT(nvh264enc), "bitrate", 5000000, NULL);
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    filesink = gst_element_factory_make("filesink", "sink");
    g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

    // Add the elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), aravissrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Link the elements
    gst_element_link_filtered(aravissrc, bayer2rgb, caps);
    gst_caps_unref(caps);
    gst_element_link_many(bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Configure aravissrc
    g_object_set(G_OBJECT(aravissrc), 
               //  "camera-name", "Basler Camera", // Adjust the camera name if needed
                 "gain", 20.0,
                 "exposure", 5000.0,
                 "packet-size", 1500,            // Set the packet size (example value)
                 "packet-delay", 1000,           // Set the packet delay (example value)
               
                 NULL);

    // Start the pipeline
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);

    // Add a bus watch to handle messages from the pipeline
    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, main_loop);
    gst_object_unref(bus);

    // Run the main loop
    g_main_loop_run(main_loop);

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Clean up
    gst_object_unref(pipeline);
    g_main_loop_unref(main_loop);

    return 0;
}