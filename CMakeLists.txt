cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)    # respect <PACKAGE>_ROOT variables in "find_package"
include(CMakePrintHelpers)

project(GrabberBaslerMultipleCameras VERSION 0.1 LANGUAGES C CXX)
# set(CMAKE_GENERATOR "Ninja")
# if (CMAKE_BUILD_TYPE)
#     unset(CMAKE_BUILD_TYPE CACHE)
# endif()
# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug, Release)")
message("CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
        mainGrabMultiCams.cpp
        src/BaslerMultipleCameras.cpp    
)
find_package(pylon 7.4.0 REQUIRED)

# Boost 
find_package(Boost REQUIRED)
find_package(CUDA REQUIRED)
# include_directories(${CUDA_INCLUDE_DIRS})


SET(PROJECT_INCLUDE_DIRECTORIES  ${CMAKE_SOURCE_DIR}/include)


# set(NVCODEC_PUBLIC_INTERFACE_DIR "C:/Users/hamit/Softwares/Video_Codec_SDK_12.2.72/Interface")
# set(NVENCODEAPI_LIB  ${NVCODEC_PUBLIC_INTERFACE_DIR}/../Lib/x64/nvencodeapi.lib)

set(GrabBaslerMultiCams mainGrabBaslerMultiCams${CMAKE_BUILD_TYPE})
add_executable(${GrabBaslerMultiCams}  ${PROJECT_SOURCES})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${GrabBaslerMultiCams}  PRIVATE DEBUG)
endif()
target_include_directories(${GrabBaslerMultiCams}  PRIVATE  ${PROJECT_INCLUDE_DIRECTORIES} ${Boost_INCLUDE_DIRS} ${ffmpeg_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS} )
target_link_libraries(${GrabBaslerMultiCams}  PRIVATE  pylon::pylon  avformat avcodec swscale avutil)

add_executable( mainGrabUsingPTP mainGrabUsingPTP.cpp)
target_link_libraries( mainGrabUsingPTP PRIVATE pylon::pylon )
install( TARGETS mainGrabUsingPTP )

