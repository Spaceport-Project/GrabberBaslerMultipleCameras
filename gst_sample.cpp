#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <stdio.h>
#include <string.h>

// Dummy function to simulate getting a buffer from an industrial camera SDK
// Replace this with your actual SDK buffer retrieval method
gboolean get_camera_buffer(guchar *buffer, gsize *size) {
    // Simulate a 4096x3000 bayer8 image buffer
    static int frame_count = 0;
    if (frame_count < 10) {  // Simulate 10 frames
        memset(buffer, frame_count % 256, 12288000);  // Dummy data
        *size = 12288000;
        frame_count++;
        return TRUE;
    }
    return FALSE;  // No more frames
}

// Function to handle messages from the GStreamer bus
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End-of-stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

// Function to push buffers into the pipeline
static void buffer_push_thread_func(GstElement *appsrc) {
    gboolean running = TRUE;
    while (running) {
        guchar buffer[12288000];
        gsize size;

        if (get_camera_buffer(buffer, &size)) {
            GstBuffer *gst_buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, buffer, size, 0, size, NULL, NULL);

            GstFlowReturn flow_ret;
            g_signal_emit_by_name(appsrc, "push-buffer", gst_buffer, &flow_ret);
            gst_buffer_unref(gst_buffer);

            if (flow_ret != GST_FLOW_OK) {
                g_printerr("Failed to push buffer to appsrc.\n");
                running = FALSE;
            }
        } else {
            // End of stream
            g_signal_emit_by_name(appsrc, "end-of-stream", NULL);
            running = FALSE;
        }
    }
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *appsrc, *bayer2rgb, *nvvideoconvert, *nvh264enc, *h264parse, *filesink;
    GstCaps *caps;
    GstBus *bus;
    GMainLoop *loop;
    GstStateChangeReturn ret;

    gst_init(&argc, &argv);

    pipeline = gst_pipeline_new("my-pipeline");
    appsrc = gst_element_factory_make("appsrc", "source");
    bayer2rgb = gst_element_factory_make("bayer2rgb", "bayer2rgb");
    nvvideoconvert = gst_element_factory_make("nvvideoconvert", "nvvideoconvert");
    nvh264enc = gst_element_factory_make("nvv4l2h264enc", "encoder");
    h264parse = gst_element_factory_make("h264parse", "h264parse");
    filesink = gst_element_factory_make("filesink", "sink");

    if (!pipeline || !appsrc || !bayer2rgb || !nvvideoconvert || !nvh264enc || !h264parse || !filesink) {
        g_printerr("Not all elements could be created.\n");
        return -1;
    }

    caps = gst_caps_from_string("video/x-bayer,width=4096,height=3000,framerate=9/1,format=rggb");
    g_object_set(G_OBJECT(appsrc), "caps", caps, "format", GST_FORMAT_TIME, NULL);
    g_object_set(G_OBJECT(nvh264enc), "bitrate", 5000000, NULL);
    g_object_set(G_OBJECT(filesink), "location", "output.mp4", NULL);

    gst_bin_add_many(GST_BIN(pipeline), appsrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL);

    if (!gst_element_link_many(appsrc, bayer2rgb, nvvideoconvert, nvh264enc, h264parse, filesink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    
    }

    // Create a GLib Main Loop
    loop = g_main_loop_new(NULL, FALSE);

    // Add a bus watch to the pipeline's bus
    bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    // Set the pipeline to the playing state
    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Failed to start the pipeline.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Push buffers into the pipeline in a thread
    g_thread_new("buffer-push-thread", (GThreadFunc)buffer_push_thread_func, appsrc);

    // Start the GLib Main Loop
    g_main_loop_run(loop);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return 0;
}