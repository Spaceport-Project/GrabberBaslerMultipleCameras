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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


message("${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")
# set(NVCODEC_UTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Utils)
# set(NV_CODEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec)
# set(NV_ENC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/NvEncoder)
# set(NV_DEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/NvDecoder)
# SET(PROJECT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include/Common ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP)
list(APPEND PROJECT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include/Common)
list(APPEND PROJECT_INCLUDE_DIRECTORIES  ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP)
# string(APPEND PROJECT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include/Common " ")
# string(APPEND PROJECT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP)
message("${PROJECT_INCLUDE_DIRECTORIES}")

set(ENCODE_SOURCE
${CMAKE_SOURCE_DIR}/AppEncCuda.cpp
)
set(ENCODEPERF_SOURCE
${CMAKE_SOURCE_DIR}/AppEncPerf.cpp
)

# set(NV_ENC_SOURCES
#  ${CMAKE_SOURCE_DIR}/src/NvEncoder.cpp
#  ${CMAKE_SOURCE_DIR}/src/NvEncoderCuda.cpp
#  ${CMAKE_SOURCE_DIR}/src/NvEncoderOutputInVidMemCuda.cpp
# )



set(PROJECT_SOURCES
        mainGrabMultiCams.cpp
        src/BaslerMultipleCameras.cpp 
        # src/Bayer2H264ConverterNvidiaCodec.cpp
        # src/Bayer2H264ConverterGST.cpp
        src/memcpy_func.cpp  
)

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_80,code=\"sm_80,compute_80\")
# if ( CMAKE_COMPILER_IS_GNUCC )
#     if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
#         list(APPEND CUDA_NVCC_FLAGS -std=c++11)
#     endif()
# endif()

find_library(CUVID_LIB nvcuvid)
find_library(NVENCODEAPI_LIB nvidia-encode)
list(APPEND CUDA_NVCC_FLAGS "-std=c++17")
find_package(pylon 7.4.0 REQUIRED)

# Boost 
find_package(OpenCV  REQUIRED)
find_package(Boost REQUIRED)
find_package(CUDA REQUIRED)




# set(CUDAToolkit_ROOT "/usr/local/cuda")
# set(CUDAToolkit_BIN_DIR "/usr/local/cuda/bin")
# set(CUDA_INCLUDE_DIRS "/usr/local/cuda/include")

find_package(PkgConfig REQUIRED)
# pkg_check_modules(gtk3 REQUIRED IMPORTED_TARGET gtk+-3.0)
# pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
# pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
# pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
# pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
# set(DEPENDENCIES "aravis-0.8")   

pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDENCIES})  
# pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
# include_directories(${GTK3_INCLUDE_DIRS})

pkg_check_modules(GSTLIBS REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-webrtc-1.0
        gstreamer-sdp-1.0
        gstreamer-pbutils-1.0
        libsoup-2.4
        json-glib-1.0
        gstreamer-rtsp-server-1.0
        gstreamer-check-1.0        
        )

message("GST Inluce ${GST_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS} ${GSTLIBS_LIBRARY_DIRS}")

# SET(PROJECT_INCLUDE_DIRECTORIES  ${CMAKE_SOURCE_DIR}/include  ${CMAKE_SOURCE_DIR}/include/Common  ${CMAKE_SOURCE_DIR}/include/Common/UtilNPP)
# SET (concurrent_queue_include "/home/spaceport/Softwares/concurrentqueue")

# set(NVCODEC_PUBLIC_INTERFACE_DIR "C:/Users/hamit/Softwares/Video_Codec_SDK_12.2.72/Interface")
# set(NVENCODEAPI_LIB  ${NVCODEC_PUBLIC_INTERFACE_DIR}/../Lib/x64/nvencodeapi.lib)
include_directories(
        /usr/lib/x86_64-linux-gnu/glib-2.0/include
        /usr/include/glib-2.0
        /usr/include/gstreamer-1.0
        /usr/include/libsoup-2.4
        /usr/include/json-glib-1.0)

link_directories(${GSTLIBS_LIBRARY_DIRS})

# set(GrabBaslerMultiCams mainGrabBaslerMultiCams${CMAKE_BUILD_TYPE})
# add_executable(${GrabBaslerMultiCams}  ${PROJECT_SOURCES})
# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_definitions(${GrabBaslerMultiCams}  PRIVATE DEBUG)
# endif()
# target_include_directories(${GrabBaslerMultiCams}  PRIVATE  ${PROJECT_INCLUDE_DIRECTORIES} ${Boost_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${ffmpeg_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS} ${concurrent_queue_include})
# target_link_libraries(${GrabBaslerMultiCams}  PRIVATE  ${OpenCV_LIBS} pylon::pylon   
#     # PkgConfig::gtk3
#     # PkgConfig::gstreamer
#     # PkgConfig::gstreamer-sdp
#     # PkgConfig::gstreamer-app
#     # PkgConfig::gstreamer-video
#     ${GSTLIBS_LIBRARIES} avformat avcodec swscale avutil tiff tbb)

# add_executable( mainGrabUsingPTP mainGrabUsingPTP.cpp)
# target_link_libraries( mainGrabUsingPTP PRIVATE pylon::pylon )
# install( TARGETS mainGrabUsingPTP )


# add_executable( gst_sample gst_sample.cpp)
# target_link_libraries( gst_sample PRIVATE  ${GSTLIBS_LIBRARIES})
# install( TARGETS gst_sample )


# add_executable( gst_sample2 gst_sample2.cpp)
# target_link_libraries( gst_sample2 PRIVATE  ${GSTLIBS_LIBRARIES} )
# install( TARGETS gst_sample2 )


# add_executable( gst_sample3 gst_sample3.cpp)
# target_link_libraries( gst_sample3 PRIVATE  ${GSTLIBS_LIBRARIES} )
# install( TARGETS gst_sample3 )


# add_executable( gst_sample4 gst_sample4.cpp)
# target_link_libraries( gst_sample4 PRIVATE  ${GSTLIBS_LIBRARIES} )
# install( TARGETS gst_sample4 )

# add_executable( gst_sample5 gst_sample5.cpp)
# target_link_libraries( gst_sample5 PRIVATE  ${GSTLIBS_LIBRARIES} )
# install( TARGETS gst_sample5 )


# add_executable( gst_sample6 gst_sample6.cpp)
# target_link_libraries( gst_sample6 PRIVATE  ${GSTLIBS_LIBRARIES} )
# install( TARGETS gst_sample6 )


# add_executable( gst_sample7 gst_sample7.cpp)
# target_link_libraries( gst_sample7 PRIVATE  ${GSTLIBS_LIBRARIES}  PkgConfig::DEPS ${GTK3_LIBRARIES} )
# install( TARGETS gst_sample7 )


# add_executable( pylonsrc pylonsrc.cpp)
# target_link_libraries( pylonsrc PRIVATE  ${GSTLIBS_LIBRARIES}  PkgConfig::DEPS ${GTK3_LIBRARIES} )
# install( TARGETS pylonsrc )

# add_executable( ffmpeg_sample2 ffmpeg_sample2.cpp)
# target_link_libraries( ffmpeg_sample2 PRIVATE  avformat avcodec swscale avutil avdevice)
# install( TARGETS ffmpeg_sample2 )

# # add_executable( ffmpeg_sample ffmpeg_sample.cpp)
# # target_link_libraries( ffmpeg_sample PRIVATE  avformat avcodec swscale avutil avdevice)
# # install( TARGETS ffmpeg_sample )
# add_executable( ffmpeg_sample3 ffmpeg_sample3.cpp)
# target_link_libraries( ffmpeg_sample3 PRIVATE  avformat avcodec swscale avutil avdevice)
# install( TARGETS ffmpeg_sample3 )

# message("${CUDA_INCLUDE_DIRS}")

# add_executable( ffmpeg_sample4 ffmpeg_sample4.cpp)
# target_include_directories(ffmpeg_sample4 PRIVATE ${OpenCV_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIRECTORIES} ${CUDA_INCLUDE_DIRS})
# target_link_libraries( ffmpeg_sample4 PRIVATE  avformat avcodec swscale avutil avdevice freeimage tiff png ${OpenCV_LIBS}   ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} ${CUDA_LIBRARIES} )
# install( TARGETS ffmpeg_sample4 )


# cuda_add_executable(AppEncCuda  ${ENCODE_SOURCE} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})
# set_target_properties(AppEncCuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# target_include_directories(AppEncCuda PUBLIC ${CUDA_INCLUDE_DIRS}

#  ${PROJECT_INCLUDE_DIRECTORIES}
# )

# target_link_libraries(AppEncCuda ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)
# install( TARGETS AppEncCuda )

# cuda_add_executable(AppEncCuda2  ${ENCODE_SOURCE} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})
# set_target_properties(AppEncCuda2 PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# target_include_directories(AppEncCuda2 PUBLIC ${CUDA_INCLUDE_DIRS}

#  ${PROJECT_INCLUDE_DIRECTORIES}
# )

# target_link_libraries(AppEncCuda2 ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)
# install( TARGETS AppEncCuda2 )

# cuda_add_executable(AppEncPerf  ${ENCODEPERF_SOURCE} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})
# set_target_properties(AppEncPerf PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# target_include_directories(AppEncPerf PUBLIC ${CUDA_INCLUDE_DIRS}

#  ${PROJECT_INCLUDE_DIRECTORIES}
# )

# target_link_libraries(AppEncPerf ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)
# install( TARGETS AppEncPerf )

set(nppi_incls
# ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImageIO.h
# ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImagesCPU.h
# ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImagesNPP.h
# ${CMAKE_SOURCE_DIR}/include/Logger.h

)


# CUDA_ADD_LIBRARY(bayer2h264converter SHARED
#     src/Bayer2H264ConverterNvidiaCodec.cpp
#     include/Bayer2H264ConverterNvidiaCodec.h
    
# )
# target_include_directories(bayer2h264converter PUBLIC ${CUDA_INCLUDE_DIRS}

#  ${PROJECT_INCLUDE_DIRECTORIES}
# )

# target_link_libraries(bayer2h264converter ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)




# set(GrabBaslerMultiCams mainGrabBaslerMultiCams${CMAKE_BUILD_TYPE})
# cuda_add_executable(${GrabBaslerMultiCams}  ${PROJECT_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} )
# set_target_properties(${GrabBaslerMultiCams}  PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_definitions(${GrabBaslerMultiCams}  PUBLIC DEBUG)
# endif()
# target_include_directories(${GrabBaslerMultiCams}  PUBLIC  ${OpenCV_INCLUDE_DIRS}  ${CUDA_INCLUDE_DIRS}  ${PROJECT_INCLUDE_DIRECTORIES} ${GSTREAMER_INCLUDE_DIRS} ${concurrent_queue_include})
# target_link_libraries(${GrabBaslerMultiCams}      

#     ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY}  ${OpenCV_LIBS} ${GSTLIBS_LIBRARIES} pylon::pylon freeimage tbb tiff
#    )

# install( TARGETS ${GrabBaslerMultiCams} )


set(GrabBaslerMultiCams mainGrabBaslerMultiCams${CMAKE_BUILD_TYPE})
add_executable(${GrabBaslerMultiCams}  ${PROJECT_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} )
# set_target_properties(${GrabBaslerMultiCams}  PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${GrabBaslerMultiCams}  PUBLIC DEBUG)
endif()
target_include_directories(${GrabBaslerMultiCams}  PUBLIC  ${OpenCV_INCLUDE_DIRS}   ${PROJECT_INCLUDE_DIRECTORIES}  )
target_link_libraries(${GrabBaslerMultiCams}      

      avformat avcodec swscale avutil avdevice ${CMAKE_DL_LIBS} ${OpenCV_LIBS}  pylon::pylon freeimage tbb tiff
   )

install( TARGETS ${GrabBaslerMultiCams} )


add_executable(open_close_cameras  open_close_cameras.cpp )

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_definitions(${GrabBaslerMultiCams}  PUBLIC DEBUG)
# endif()
target_include_directories(${GrabBaslerMultiCams}  PUBLIC  ${OpenCV_INCLUDE_DIRS}  ${CUDA_INCLUDE_DIRS}  ${PROJECT_INCLUDE_DIRECTORIES} ${GSTREAMER_INCLUDE_DIRS} ${concurrent_queue_include})
target_link_libraries(open_close_cameras     

     pylon::pylon
   )

install( TARGETS open_close_cameras )
