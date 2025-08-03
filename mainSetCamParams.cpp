//#include <pthread.h>
#include <iostream>
#include <vector>
#include <cstdint>
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#   include <pylon/PylonGUI.h>
#endif

#include <pylon/BaslerUniversalInstantCameraArray.h>
// #include <pylon/gige/PylonGigEIncludes.h>
#include <pylon/Info.h>
#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/ActionTriggerConfiguration.h>
#include <pylon/gige/BaslerGigEDeviceInfo.h>
#include "magic_enum.hpp"

using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;




int ConfigureCameraSettings(const std::string & cameraSettingsFile )
{
    boost::property_tree::ptree pt;

    // Read the JSON file
    int m_nExitCode = 0;
    std::ifstream file(cameraSettingsFile);
    if (!file.good()) 
    {
        printf("Error in opening 'CameraSettings.json' file! Exiting... \n");
        m_nExitCode = 1;
        return m_nExitCode;
    }

    boost::property_tree::read_json(file, pt);
    unsigned int m_uHeight = pt.get<unsigned int>("Height");
    unsigned int m_uWidth = pt.get<unsigned int>("Width");
    float m_fResizeFactor_= pt.get<float>("ResizeFactor");
    int m_uFullResCntLimit =  pt.get<int>("FullResCountLimit");
    float m_fExposureTime = pt.get<float>("ExposureTime");
    float m_fAcquisitionFrameRate = pt.get<float>("AcquisitionFrameRate");
    float m_fGain = pt.get<float>("Gain");
    unsigned int m_uFrameNum = pt.get<unsigned int>("FrameNum");
    std::string m_sPixelFormat = pt.get<std::string>("PixelFormat");
    unsigned int m_uPacketSize =  pt.get<unsigned int>("PacketSize");
    unsigned int m_uTransDelay = pt.get<unsigned int>("TransDelay");
    unsigned int m_uInterPacketDelay = pt.get<unsigned int>("IntPacketDelay");
  
    
    auto pixelFormatEnum = magic_enum::enum_cast<Basler_UniversalCameraParams::PixelFormatEnums>(m_sPixelFormat);
  
    file.close();

    CTlFactory &tlFactory = CTlFactory::GetInstance();
    DeviceInfoList_t allDeviceInfos;
    CBaslerUniversalInstantCameraArray bsCameras;
    IGigETransportLayer* pTL=nullptr;

    try
    {
        // Get the transport layer factory.
        pTL = dynamic_cast<IGigETransportLayer*>(tlFactory.CreateTl( BaslerGigEDeviceClass ));
    
        if (pTL->EnumerateDevices( allDeviceInfos ) == 0)
        {
            throw RUNTIME_EXCEPTION( "No GigE cameras present!" );
        }
        // Get all attached cameras.
      
    }
    catch (const GenericException& e)
    {
        PYLON_UNUSED( e );

        std::cerr<<e.GetDescription()<<std::endl;
    }
    int DeviceNum = allDeviceInfos.size() ;
    // DeviceNum = m_uNumCams <= m_uDeviceNum ? m_uNumCams : m_uDeviceNum;
    std::cout<<DeviceNum<<" GigE Cameras Found!"<<std::endl;
    // for (unsigned int i = 0; i < DeviceNum; i++) {
    //     std::cout<<i+1<<".Cam Serial Num:"<<allDeviceInfos[i].GetSerialNumber().c_str()<<std::endl;
    //     // m_mapSerials.insert(std::make_pair(i , allDeviceInfos[i].GetSerialNumber().c_str()));
 
    // }  
    bsCameras.Initialize(DeviceNum);

    try
    {
        for (size_t i = 0; i < DeviceNum; ++i)
        {
            bsCameras[i].Attach( tlFactory.CreateDevice( allDeviceInfos[i] ) );
            // bsCameras[i].RegisterImageEventHandler( new CBaslerImageEventHandler(m_queueGrabRes), RegistrationMode_Append, Cleanup_Delete );
            bsCameras[i].GrabCameraEvents = true;
            bsCameras[i].SetCameraContext( i );
            
            bsCameras[i].Open();
            if (!bsCameras[i].EventSelector.IsWritable())
            {
                throw RUNTIME_EXCEPTION( "The device doesn't support events." );
            }

            std::cout<<i+1<<".Cam Serial Num:"<<allDeviceInfos[i].GetSerialNumber().c_str()<<" has opened!"<<std::endl;

        }
    }  
    catch (const GenericException& e)
    {
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
        return m_nExitCode;
    }
    

    try
    {
        for (size_t i = 0; i < DeviceNum; ++i)
        {
            
           
            bsCameras[i].UserSetSelector.SetValue("Default");
            bsCameras[i].UserSetLoad.Execute();

          
            if (pixelFormatEnum.has_value())
                bsCameras[i].PixelFormat.SetValue(pixelFormatEnum.value());
            else 
                std::cerr << "Pixel Format not set! Continueing with the default or previous value." << std::endl;

           
            while (bsCameras[i].GevSCPSPacketSize.GetValue()!= m_uPacketSize) {
				bsCameras[i].GevSCPSPacketSize.SetValue(m_uPacketSize);
			}
            bsCameras[i].GevSCPD.SetValue(m_uTransDelay);
            bsCameras[i].GevSCFTD.SetValue(m_uInterPacketDelay);
            bsCameras[i].BandwidthReserveMode.SetValue(BandwidthReserveMode_Performance);
            // BandwidthReserveModeEnums e = bsCameras[i].BandwidthReserveMode.GetValue();
            // std::cout<<"e:"<<e<<std::endl;
            bsCameras[i].Width.SetValue(m_uWidth);
            if (m_uWidth == 3500) bsCameras[i].OffsetX.SetValue(296);
			bsCameras[i].Height.SetValue(m_uHeight);
            bsCameras[i].ExposureTime.SetValue(m_fExposureTime);
            bsCameras[i].GainSelector.SetValue(GainSelector_All);
            bsCameras[i].Gain.SetValue(m_fGain);


         
            
            // cout<<"ptp clock:"<<bsCameras[i].BslPeriodicSignalSource.GetValue()<<endl;
            // bsCameras[i].PtpEnable.SetValue(false);
            // if (i == 0)
            //     bsCameras[i].BslPtpPriority1.SetValue(0);
            // else 
            //     bsCameras[i].BslPtpPriority1.SetValue(255);
            // // Enable end-to-end delay measurement
            // bsCameras[i].BslPtpProfile.SetValue(BslPtpProfile_DelayRequestResponseDefaultProfile);
            // // Set the network mode to unicast
            // bsCameras[i].BslPtpNetworkMode.SetValue(BslPtpNetworkMode_Multicast);
            // bsCameras[i].BslPtpManagementEnable.SetValue(true);
            // bsCameras[i].PtpEnable.SetValue(false);
            // // Disable two-step operation
            // bsCameras[i].BslPtpTwoStep.SetValue(false);
            // std::cout<<"PTP enabled:"<<bsCameras[i].PtpEnable.GetValue()<<std::endl;
            bsCameras[i].PtpEnable.SetValue(true);

           

            // std::cout<<i<<".cam IEEE1588 status:"<<bsCameras[i].PtpStatus.GetValue()<< " "<< bsCameras[i].TriggerSource.GetValue()<<std::endl;
           
            bsCameras[i].BslPeriodicSignalPeriod.SetValue(1/m_fAcquisitionFrameRate  * 1e6);
            bsCameras[i].BslPeriodicSignalDelay.SetValue(0);
            bsCameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
            bsCameras[i].TriggerMode.SetValue(TriggerMode_On);
            bsCameras[i].TriggerSource.SetValue(TriggerSource_PeriodicSignal1);
            if (bsCameras[i].BslPeriodicSignalSource.GetValue() != BslPeriodicSignalSource_PtpClock ){
               printf("Clock source of periodic signal is not `PtpClock`\n");
               return 0;
            }

        }


    }  
    catch (const GenericException& e)
    {
        // Error handling
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
        return m_nExitCode;
    }

    try {
        bsCameras.StopGrabbing(); 
        std::cout<<"Stoping success!"<<std::endl;
    }
    catch (const GenericException& e)
    {
        // Error handling
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        m_nExitCode = 1;
    }

    
    return m_nExitCode;
    
   
}



int main(int argc, char *argv[]) {
    int exit_code = 0;
    if (argc != 2) {
        printf("Usage: %s <path/to/CameraSettings.json>\n", argv[0]);

        return -1;

    } 
    // gst_init(&argc, &argv);

   

    std::string cameraSettingsFile(argv[1]);

    // std::thread memoryChecker(checkMemoryUsage);

    // memoryChecker.detach();


    PylonInitialize();
    
    ConfigureCameraSettings(cameraSettingsFile);
   
    PylonTerminate();





    return 0;


}
