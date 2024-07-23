#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <atomic>
#include <map>
// #include <cuda.h>
// #include <cuda_runtime.h>
#include <mutex>

#include <gst/gst.h>
#include <gst/app/app.h>
#include <pylon/BaslerUniversalInstantCameraArray.h>
// #include <pylon/gige/PylonGigEIncludes.h>
#include <pylon/Info.h>
#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/ActionTriggerConfiguration.h>
#include <pylon/gige/BaslerGigEDeviceInfo.h>
// #include <NvEncodeAPI.h>
using namespace Basler_UniversalCameraParams;

using namespace Pylon;
using namespace GenApi;
struct DATA{
 
    // std::shared_ptr<uint8_t[]>  image;
    u_int8_t *image;
    // std::vector<u_int8_t> image;
    u_int64_t timeStamp=0;
    size_t imageSize=0;
    std::string serialNumber;
    // guint cam_index=0;
    // GstElement *appsrc = nullptr;
    // GstElement *pipeline = nullptr;
    // CBaslerUniversalInstantCamera *cam;
};


class BayerToH264ConverterGST{
public:
    BayerToH264ConverterGST(std::map<int, std::string> mapSerialNums, unsigned int input_width, unsigned int input_height);
    ~BayerToH264ConverterGST();
    void InitializeAllGstPipelines();
    void InitializeSingleGstPipeline(unsigned int /*DATA *data_struct*/);
    bool CloseSingleGstPipeline(unsigned int);
    bool StartSingleGstPipeline(unsigned int n_cam_index);

    bool CloseAllGstPipelines();
    gboolean push_data( guint8 *camera_sdk_buffer, unsigned int n_cam_index);
    gboolean push_data2( guint8 *camera_sdk_buffer,  unsigned int n_cam_index);
    static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);
    static void on_need_callback(GstElement *appsrc, guint unused_size, DATA *data_struct);
    static bool m_bExit;

    // static  void
    // start_feed (GstElement * pipeline, guint size, DATA* );

private:
    const unsigned int width_;
    const unsigned int height_;
    const unsigned int size_;

    unsigned int num_devices_;
    unsigned int frameIndex = 0;
    std::vector<bool> results_;
    std::vector<std::mutex> codecMutexes_;
    std::vector< unsigned int>  frame_cnts;
    std::ofstream outputFile;
    std::map<int, std::string> mapSerialNums_;
    // std::vector<DATA*> datas_;
    std::vector<DATA> data_struct;
    unsigned int frameDurationInMS;

    std::vector<GstElement*> pipelines_, sources_, capsfilters1_, capsfilters2_, capsfilters3_, bayer_to_rgb_, vidconvs_,nvvidconvs_, encoders_, h264parse_, muxers_, sinks_;
    std::vector<GstBus*> buses_;
    std::vector<GstMessage *> msgs_;
    std::vector<GstStateChangeReturn> rets_;
    std::vector<GMainLoop *> main_loops_;
    // GstCaps * caps_;
    std::vector< GstCaps *> caps1_, caps2_, caps3_;

    
};
       
