#include <time.h>   // for time
#include <stdlib.h> // for rand & srand
#include <thread>

// Include files to use the pylon API.
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
#include <unistd.h>


// Namespace for using pylon universal instant camera parameters.
using namespace Basler_UniversalCameraParams;

// using namespace Basler_GigECameraParams;

// Namespace for using pylon objects.
using namespace Pylon;
using namespace GenApi;


// Namespace for using cout.
using namespace std;

// Limits the amount of cameras used for grabbing.
// It is important to manage the available bandwidth when grabbing with multiple
// cameras. This applies, for instance, if two GigE cameras are connected to the
// same network adapter via a switch. To manage the bandwidth, the GevSCPD
// interpacket delay parameter and the GevSCFTD transmission delay parameter can
// be set for each GigE camera device. The "Controlling Packet Transmission Timing
// with the Interpacket and Frame Transmission Delays on Basler GigE Vision Cameras"
// Application Note (AW000649xx000) provides more information about this topic.
static const uint32_t c_maxCamerasToUse = 2;

int main( int /*argc*/, char* /*argv*/[] )
{
    int exitCode = 0;

    // Before using any pylon methods, the pylon runtime must be initialized.
    PylonInitialize();

    try
    {
        // Get the GigE transport layer.
        // We'll need it later to issue the action commands.
        
        CTlFactory& tlFactory = CTlFactory::GetInstance();
        IGigETransportLayer* pTL = dynamic_cast<IGigETransportLayer*>(tlFactory.CreateTl( BaslerGigEDeviceClass ));
        if (pTL == NULL)
        {
            throw RUNTIME_EXCEPTION( "No GigE transport layer available." );
        }


        // In this sample we use the transport layer directly to enumerate cameras.
        // By calling EnumerateDevices on the TL we get get only GigE cameras.
        // You could also accomplish this by using a filter and
        // let the Transport Layer Factory enumerate.
        DeviceInfoList_t allDeviceInfos;
        if (pTL->EnumerateDevices( allDeviceInfos ) == 0)
        {
            throw RUNTIME_EXCEPTION( "No GigE cameras present." );
        }

        // // Only use cameras in the same subnet as the first one.
        // DeviceInfoList_t usableDeviceInfos;
        // usableDeviceInfos.push_back( allDeviceInfos[0] );
        const String_t subnet( allDeviceInfos[0].GetSubnetAddress() );

        // Start with index 1 as we have already added the first one above.
        // We will also limit the number of cameras to c_maxCamerasToUse.
        // for (size_t i = 1; i < allDeviceInfos.size(); ++i)
        // {
            
        //     // const CBaslerGigEDeviceInfo& gigeinfo = static_cast<const CBaslerGigEDeviceInfo&>(allDeviceInfos[i]);
        //     // if (subnet == gigeinfo.GetSubnetAddress())
        //     // {
        //     //     // Add this deviceInfo to the ones we will be using.
        //     //     usableDeviceInfos.push_back(gigeinfo);
        //     // }
        //     // else
        //     // {
        //     //     cerr << "Camera will not be used because it is in a different subnet " << subnet << "!" << endl;
        //     // }
            
        //     if (subnet == allDeviceInfos[i].GetSubnetAddress())
        //     {
        //         // Add this deviceInfo to the ones we will be using.
        //         usableDeviceInfos.push_back( allDeviceInfos[i] );
        //     }
        //     else
        //     {
        //         cerr << "Camera will not be used because it is in a different subnet "
        //             << subnet << "!" << endl;
        //     }
        // }

        // In this sample we'll use an CBaslerGigEInstantCameraArray to access multiple cameras.
        CBaslerUniversalInstantCameraArray cameras( allDeviceInfos.size() );
        // CBaslerGigEInstantCameraArray cameras( usableDeviceInfos.size() );
        return 1;
        // Seed the random number generator and generate a random device key value.
        srand( (unsigned) time( NULL ) );
        const uint32_t DeviceKey = rand();

        // For this sample we configure all cameras to be in the same group.
        const uint32_t GroupKey = 0x112233;

        // For the following sample we use the CActionTriggerConfiguration to configure the camera.
        // It will set the DeviceKey, GroupKey and GroupMask features. It will also
        // configure the camera FrameTrigger and set the TriggerSource to the action command.
        // You can look at the implementation of CActionTriggerConfiguration in <pylon/gige/ActionTriggerConfiguration.h>
        // to see which features are set.

        
        for (size_t i = 0; i < cameras.GetSize(); ++i)
        {
            cameras[i].Attach( tlFactory.CreateDevice( allDeviceInfos[i] ) );

           
            // // We'll use the CActionTriggerConfiguration, which will set up the cameras to wait for an action command.
            cameras[i].RegisterConfiguration( new CActionTriggerConfiguration( DeviceKey, GroupKey, AllGroupMask ), RegistrationMode_Append, Cleanup_Delete );
            // // Set the context. This will help us later to correlate the grab result to a camera in the array.
            cameras[i].SetCameraContext( i );
            cameras[i].Open();
            // cameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
            // // Select the mode for the selected trigger
            // cameras[i].TriggerMode.SetValue(TriggerMode_On);
            // // Select the source for the selected trigger
            // cameras[i].TriggerSource.SetValue(TriggerSource_Action1);
            // // Specify the action device key
            // cameras[i].ActionDeviceKey.SetValue(DeviceKey);
            // // In this example, all cameras will be in the same group
            // cameras[i].ActionGroupKey.SetValue(GroupKey);
            // // Specify the action group mask
            // cameras[i].ActionGroupMask.SetValue(AllGroupMask);
            cameras[i].PixelFormat.SetValue(PixelFormat_BayerRG8);



            cameras[i].PtpEnable.SetValue(false);
            
            // cameras[i].GevIEEE1588.SetValue(true);

            cameras[i].BslPtpPriority1.SetValue(128);
            // Enable end-to-end delay measurement
            cameras[i].BslPtpProfile.SetValue(BslPtpProfile_DelayRequestResponseDefaultProfile);
            // Set the network mode to unicast
            cameras[i].BslPtpNetworkMode.SetValue(BslPtpNetworkMode_Multicast);
            // Set the IP address of the first unicast device to 192.168.10.12
            // (0xC0 = 192, 0xA8 = 168, 0x0A = 10, 0x0C = 12)
            // cameras[i].BslPtpUcPortAddrIndex.SetValue(0);
            // cameras[i].BslPtpUcPortAddr.SetValue(0xC0A8125E);
            // Enable PTP Management Protocol
            cameras[i].BslPtpManagementEnable.SetValue(true);
            // Disable two-step operation
            cameras[i].BslPtpTwoStep.SetValue(false);
            cameras[i].PtpEnable.SetValue(true);


            cameras[i].GevSCPSPacketSize.SetValue(8000);
			cameras[i].GevSCPD.SetValue(2500);

            while (cameras[i].GevSCPSPacketSize.GetValue()!=8000) {
				cameras[i].GevSCPSPacketSize.SetValue(8000);
			}
			// cameras[i].MaxNumBuffer.SetValue(20);

			// cameras[i].ExposureTimeAbs.SetValue(6000);
			// cameras[i].GainRaw.SetValue(20);

		

			cameras[i].Width.SetValue(4096);
			cameras[i].Height.SetValue(3000);
            cameras[i].ExposureTime.SetValue(5000.0);
            cameras[i].GainSelector.SetValue(GainSelector_All);
            cameras[i].Gain.SetValue(24.000);

            if (cameras[i].BslPeriodicSignalSource.GetValue() != BslPeriodicSignalSource_PtpClock ){
               printf("Clock source of periodic signal is not `PtpClock`\n");
               return 0;
            }
            // cout<<"ptp clock:"<<cameras[i].BslPeriodicSignalSource.GetValue()<<endl;

            cameras[i].BslPeriodicSignalPeriod.SetValue(1 / 3. * 1e6);
            cameras[i].BslPeriodicSignalDelay.SetValue(0);
            cameras[i].TriggerSelector.SetValue(TriggerSelector_FrameStart);
            cameras[i].TriggerMode.SetValue(TriggerMode_On);
            cameras[i].TriggerSource.SetValue(TriggerSource_PeriodicSignal1);

           

            const CBaslerGigEDeviceInfo& di = cameras[i].GetDeviceInfo();

            // Print the model name of the camera.
            cout << "Using camera " << i << ": " << di.GetModelName() << " (" << di.GetIpAddress() << ")" << endl;
        }

        
        cameras.StartGrabbing();

        
        //
        cameras[0].TimestampLatch.Execute();
        uint64_t currentTimestamp = cameras[0].TimestampLatchValue.GetValue();
          

        int64_t actionTime = currentTimestamp;
        uint32_t c_countOfImagesToGrab=10;


        for (uint32_t i = 0; i < c_countOfImagesToGrab && cameras.IsGrabbing(); i++) {
        // This smart pointer will receive the grab result data.
            CBaslerUniversalGrabResultPtr ptrGrabResult;
            // actionTime  += 500000000;
            // pTL->IssueActionCommand(DeviceKey, GroupKey, AllGroupMask, subnet );
            // pTL->IssueScheduledActionCommand(DeviceKey, GroupKey, AllGroupMask, actionTime);
            // pTL->IssueScheduledActionCommand(4711, 1, 0xffffffff, actionTime, "192.168.18.255");



            // Retrieve images from all cameras.
            const int DefaultTimeout_ms = 5000;
            for (size_t j = 0; j < allDeviceInfos.size() && cameras.IsGrabbing(); ++j)
            {
                // CInstantCameraArray::RetrieveResult will return grab results in the order they arrive.
                cameras.RetrieveResult( DefaultTimeout_ms, ptrGrabResult, TimeoutHandling_ThrowException );

                // When the cameras in the array are created the camera context value
                // is set to the index of the camera in the array.
                // The camera context is a user-settable value.
                // This value is attached to each grab result and can be used
                // to determine the camera that produced the grab result.
                intptr_t cameraIndex = ptrGrabResult->GetCameraContext();


                // Image grabbed successfully?
                if (ptrGrabResult->GrabSucceeded())
                {
    #ifdef PYLON_WIN_BUILD
                    // Show the image acquired by each camera in the window related to the camera.
                    // DisplayImage supports up to 32 image windows.
                    if (cameraIndex <= 31)
                        Pylon::DisplayImage( cameraIndex, ptrGrabResult );
    #endif
                    cout<<"Timestamp:"<<ptrGrabResult->GetTimeStamp()<<endl;
                    // Print the index and the model name of the camera.
                    cout << "Camera " << cameraIndex << ": " << cameras[cameraIndex].GetDeviceInfo().GetModelName() <<
                        " (" << cameras[cameraIndex].GetDeviceInfo().GetIpAddress() << ")" << endl;

                    // You could process the image here by accessing the image buffer.
                    cout << "GrabSucceeded: " << ptrGrabResult->GrabSucceeded() << endl;
                    const uint8_t* pImageBuffer = (uint8_t*) ptrGrabResult->GetBuffer();
                    cout << "Gray value of first pixel: " << (uint32_t) pImageBuffer[0] << endl << endl;
                    std::string filename= "GrabbedImage_" + std::to_string (i) + ".raw";
                    String_t file_name(filename.c_str());
                    cout<<file_name<<endl;
                    CImagePersistence::Save( ImageFileFormat_Raw, file_name, ptrGrabResult );
                }
                else
                {
                    // If a buffer has been incompletely grabbed, the network bandwidth is possibly insufficient for transferring
                    // multiple images simultaneously. See note above c_maxCamerasToUse.
                    cout << "Error: " << std::hex << ptrGrabResult->GetErrorCode() << std::dec << " " << ptrGrabResult->GetErrorDescription() << endl;
                }
            }
        }

        // In case you want to trigger again you should wait for the camera
        // to become trigger-ready before issuing the next action command.
        // To avoid overtriggering you should call cameras[0].WaitForFrameTriggerReady
        // (see Grab_UsingGrabLoopThread sample for details).

        cameras.StopGrabbing();

        // Close all cameras.
        cameras.Close();
    }
    catch (const GenericException& e)
    {
        // Error handling
        cerr << "An exception occurred." << endl
            << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Comment the following two lines to disable waiting on exit.
    cerr << endl << "Press enter to exit." << endl;
    while (cin.get() != '\n');

    // Releases all pylon resources.
    PylonTerminate();

    return exitCode;
}


