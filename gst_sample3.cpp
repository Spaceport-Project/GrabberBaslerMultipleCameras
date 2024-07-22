#include <gst/gst.h>
#include <string.h>

static gboolean push_data(GstElement *appsrc) {
    static guint64 timestamp = 0;
    GstBuffer *buffer;
    GstFlowReturn ret;
    guint size = 12288000; // Adjust the size according to your buffer size
    GstMapInfo map;

    // Allocate a new buffer
    buffer = gst_buffer_new_allocate(NULL, size, NULL);

    // Map the buffer and fill it with data from your camera SDK
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    // Here you should copy your camera SDK buffer into map.data
    // For example: memcpy(map.data, your_camera_sdk_buffer, size);
    memset(map.data, 0xff, size); // Dummy data for example
    gst_buffer_unmap(buffer, &map);

    // Set the buffer timestamp
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 9); // Adjust the framerate
    timestamp += GST_BUFFER_DURATION(buffer);

    // Push the buffer into the appsrc
    g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);

    // Free the buffer
    gst_buffer_unref(buffer);

    if (ret != GST_FLOW_OK) {
        // We got some error, stop sending data
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *appsrc, *bayer2rgb, *nvvideoconvert, *nvh264enc, *h264parse, *filesink;
    GstCaps *caps;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    GMainLoop *main_loop;

    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    pipeline = gst_pipeline_new("my-pipeline");

    // Create the elements
    appsrc = gst_element_factory_make("appsrc", "source");
    caps = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=9/1,format=rggb");
    bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    nvh264enc = gst_element_factory_make("nvv4l2h264enc", "encoder");
    g_object_set(G_OBJECT(nvh264enc), "bitrate", 5000000, NULL);
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    filesink = gst_element_factory_make("filesink", "sink");
    g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

    // Add the elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), appsrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Link the elements
    gst_element_link_filtered(appsrc, bayer2rgb, caps);
    gst_caps_unref(caps);
    gst_element_link_many(bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    // Configure appsrc
    g_object_set(G_OBJECT(appsrc), "caps", caps, "format", GST_FORMAT_TIME, NULL);

    // Start the pipeline
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Create a GLib Main Loop and set it to run
    main_loop = g_main_loop_new(NULL, FALSE);

    // Add a timeout function to push data into appsrc
    g_timeout_add(1000 / 9, (GSourceFunc)push_data, appsrc); // Adjust the interval according to your framerate

    // Run the loop
    g_main_loop_run(main_loop);

    // Wait until the pipeline finishes
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    gst_element_send_event(pipeline, gst_event_new_eos());

    // Stop the pipeline
    gst_element_set_state(pipeline, GST_STATE_NULL);

    // Clean up
    gst_object_unref(msg);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(main_loop);

    return 0;
}