
// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCameraArray.h>

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <pylon/gige/GigETransportLayer.h>
#include <pylon/gige/ActionTriggerConfiguration.h>
#include <pylon/gige/BaslerGigEDeviceInfo.h>
// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 10;


int main(){
        PylonInitialize();

        CTlFactory& tlFactory = CTlFactory::GetInstance();
         DeviceInfoList_t devices;
        try
        {
            // Get the transport layer factory.
             IGigETransportLayer* m_pTL = dynamic_cast<IGigETransportLayer*>(tlFactory.CreateTl( BaslerGigEDeviceClass ));
        
            if (m_pTL->EnumerateDevices( devices ) == 0)
            {
                throw RUNTIME_EXCEPTION( "No GigE cameras present!" );
            }
            // Get all attached cameras.
            // m_tlFactory.EnumerateDevices( m_allDeviceInfos);
        }
        catch (const GenericException& e)
        {
            PYLON_UNUSED( e );

            std::cerr<<e.GetDescription()<<std::endl;
        }
        // Get all attached devices and exit application if no device is found.
        // DeviceInfoList_t devices;
        // if (tlFactory.EnumerateDevices( devices ) == 0)
        // {
        //     throw RUNTIME_EXCEPTION( "No camera present." );
        // }

        // Create an array of instant cameras for the found devices and avoid exceeding a maximum number of devices.
        CInstantCameraArray cameras( devices.size() );
        // CBaslerUniversalInstantCameraArray cameras(devices.size());

        // Create and attach all Pylon Devices.
        for (size_t i = 0; i < cameras.GetSize(); ++i)
        {
            cameras[i].Attach( tlFactory.CreateDevice( devices[i] ) );

            // Print the model name of the camera.
            cout << "Using device : " <<i<<" .cam "<< cameras[i].GetDeviceInfo().GetModelName() << endl;
            cameras[i].Open();
            cameras[i].Close(); 

        }

        PylonTerminate();


}
