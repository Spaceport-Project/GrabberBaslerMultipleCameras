cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0074 NEW)  
include(CMakePrintHelpers)

project(GrabberBaslerMultipleCameras VERSION 0.1 LANGUAGES C CXX)

message("CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


message("${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJECT_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")

list(APPEND PROJECT_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include/Common)
list(APPEND PROJECT_INCLUDE_DIRECTORIES  ${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP)
message("${PROJECT_INCLUDE_DIRECTORIES}")

set(ENCODE_SOURCE
${CMAKE_SOURCE_DIR}/AppEncCuda.cpp
)
set(ENCODEPERF_SOURCE
${CMAKE_SOURCE_DIR}/AppEncPerf.cpp
)

set(NV_ENC_SOURCES
 ${CMAKE_SOURCE_DIR}/src/NvEncoder.cpp
 ${CMAKE_SOURCE_DIR}/src/NvEncoderCuda.cpp
 ${CMAKE_SOURCE_DIR}/src/NvEncoderOutputInVidMemCuda.cpp
)

set(NV_ENC_CUDA_UTILS
${CMAKE_SOURCE_DIR}/src/crc.cu
)


set(PROJECT_SOURCES
        mainGrabMultiCams.cpp
        src/BaslerMultipleCameras.cpp 
        src/Bayer2H264ConverterNvidiaCodec.cpp
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




# add_executable( mainGrabUsingPTP mainGrabUsingPTP.cpp)
# target_link_libraries( mainGrabUsingPTP PRIVATE pylon::pylon )
# install( TARGETS mainGrabUsingPTP )






cuda_add_executable(AppEncCuda  ${ENCODE_SOURCE} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})
set_target_properties(AppEncCuda PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
target_include_directories(AppEncCuda PUBLIC ${CUDA_INCLUDE_DIRS}

 ${PROJECT_INCLUDE_DIRECTORIES}
)

target_link_libraries(AppEncCuda ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)
install( TARGETS AppEncCuda )


# cuda_add_executable(AppEncPerf  ${ENCODEPERF_SOURCE} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})
# set_target_properties(AppEncPerf PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
# target_include_directories(AppEncPerf PUBLIC ${CUDA_INCLUDE_DIRS}

#  ${PROJECT_INCLUDE_DIRECTORIES}
# )

# target_link_libraries(AppEncPerf ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY} freeimage)
# install( TARGETS AppEncPerf )

set(nppi_incls
${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImageIO.h
${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImagesCPU.h
${CMAKE_CURRENT_SOURCE_DIR}/include/Common/UtilNPP/ImagesNPP.h
${CMAKE_SOURCE_DIR}/include/Logger.h

)




set(GrabBaslerMultiCams mainGrabBaslerMultiCams${CMAKE_BUILD_TYPE})
cuda_add_executable(${GrabBaslerMultiCams}  ${PROJECT_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} )
set_target_properties(${GrabBaslerMultiCams}  PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${GrabBaslerMultiCams}  PUBLIC DEBUG)
endif()
target_include_directories(${GrabBaslerMultiCams}  PUBLIC  ${OpenCV_INCLUDE_DIRS}  ${CUDA_INCLUDE_DIRS}  ${PROJECT_INCLUDE_DIRECTORIES} ${GSTREAMER_INCLUDE_DIRS} ${concurrent_queue_include})
target_link_libraries(${GrabBaslerMultiCams}      

    ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB}  ${CUDA_nppc_LIBRARY} ${CUDA_nppisu_LIBRARY} ${CUDA_nppicc_LIBRARY}  ${OpenCV_LIBS} pylon::pylon freeimage tbb tiff
   )

install( TARGETS ${GrabBaslerMultiCams} )

