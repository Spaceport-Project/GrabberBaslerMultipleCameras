#include <gst/gst.h>

int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    GstElement *pipeline = gst_pipeline_new("my-pipeline");

    // Create the elements
    GstElement *filesrc = gst_element_factory_make("filesrc", "source");
    g_object_set(G_OBJECT(filesrc), "location", "/home/hamit/Softwares/GrabberBaslerMultipleCameras/build/bayer8.bin", "blocksize", 12288000, NULL);

    GstCaps *caps = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=9/1,format=rggb");
    GstElement *bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");

    GstElement *nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    GstElement *nvh264enc = gst_element_factory_make("nvv4l2h264enc", "encoder");
    g_object_set(G_OBJECT(nvh264enc), "bitrate", 5000000, NULL);
    GstElement *h264parse = gst_element_factory_make("h264parse", "h264parse");
    GstElement *filesink = gst_element_factory_make("filesink", "sink");
    g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

    // Add the elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), filesrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Link the elements
    gst_element_link_filtered(filesrc, bayer2rgb, caps);
    gst_caps_unref(caps);
    gst_element_link_many(bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Start the pipeline
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Wait until the pipeline finishes
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Clean up
    // gst_object_unref(msg);
    gst_object_unref(bus);
    gst_object_unref(pipeline);

    return 0;
}


