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
#include <unordered_set>

using namespace Pylon;
using namespace GenApi;
using namespace Basler_UniversalCameraParams;


std::unordered_set<std::string> lines1 = {
"40455112", 
"40455113", 
"40455114", 
"40455115", 
"40455116", 
"40455117", 
"40455118", 
"40455119", 
"40455120", 
"40455121", 
"40455122", 
"40455123", 
"40455124", 
"40455125", 
"40455126", 
"40455127", 
"40455128", 
"40455129", 
"40455130", 
"40455131", 
"40455132", 
"40455133", 
"40455134", 
"40455135", 
"40608426", 
"40608427", 
"40608428", 
"40608430", 
"40608431", 
"40608432", 
"40608433", 
"40608434", 
"40608435", 
"40608439", 
"40608440", 
"40608441", 
"40608442", 
"40608443", 
"40608444", 
"40608445", 
"40608446", 
"40608447", 
"40608449", 
"40608450"
  
};




int PrintDeviceNums( )
{
   
  

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
    std::unordered_set<std::string> lines2;
    
    for (size_t i = 0; i < DeviceNum; ++i)
    {
    
        std::cout<<i+1<<".Cam Serial Num:"<<allDeviceInfos[i].GetSerialNumber().c_str()<<" "<<std::endl;
        lines2.insert(allDeviceInfos[i].GetSerialNumber().c_str());
    }

    
    // for (const auto& l : lines1) {
    //     if (lines2.find(l) == lines2.end()) {
    //         std::cout << l << std::endl;
    //     }
    // }
    std::vector<int> diff1;
    for (const auto& l : lines1) {
        if (lines2.find(l) == lines2.end()) {
            diff1.push_back(std::stoi(l));
        }
    }
    if (diff1.size() > 0 ) {
        std::cout << "\nMissing devices listed below: \n";
        std::sort(diff1.begin(), diff1.end());
        for (const auto& num : diff1) {
            std::cout << num << std::endl;
        }
    }  
   
    return 0;
    
   
}



int main(int argc, char *argv[]) {
    int exit_code = 0;
    // if (argc != 2) {
    //     printf("Usage: %s <path/to/CameraSettings.json>\n", argv[0]);

    //     return -1;

    // } 

   

    // std::string cameraSettingsFile(argv[1]);

    

    PylonInitialize();
    
    PrintDeviceNums();
   
    PylonTerminate();





    return 0;


}
