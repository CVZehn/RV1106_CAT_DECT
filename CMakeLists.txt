cmake_minimum_required(VERSION 3.10)

set(SDK_PATH $ENV{LUCKFOX_SDK_PATH})
if(NOT SDK_PATH)
    message(WARNING "Please Set Luckfox-pico SDK Path. Such as:export LUCKFOX_SDK_PATH=/home/user/luckfox-pico")
    return()
endif()


set(CMAKE_C_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc"
)
set(CMAKE_CXX_COMPILER "${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++"
)

project(MixedLanguage C CXX)
project(luckfox_rtsp_yolov5)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common 
                    ${CMAKE_CURRENT_SOURCE_DIR}/common/isp3.x   
                    ${CMAKE_CURRENT_SOURCE_DIR}/mqttlib/openssl/include   
                    ${CMAKE_CURRENT_SOURCE_DIR}/mqttlib/paho/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/allocator/dma
                    #${CMAKE_CURRENT_SOURCE_DIR}/move_detect/include 
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rga
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/uAPI2
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/common
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/xcore
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/algos
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/iq_parser
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/iq_parser_v2
                    ${CMAKE_CURRENT_SOURCE_DIR}/media_out/include/rkaiq/smartIr)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib
                 ${CMAKE_CURRENT_SOURCE_DIR}/media_out/lib 
                 ${CMAKE_CURRENT_SOURCE_DIR}/mqttlib/openssl/lib 
                 ${CMAKE_CURRENT_SOURCE_DIR}/mqttlib/paho/lib 
                 #${CMAKE_CURRENT_SOURCE_DIR}/move_detect/lib
                )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils utils.out)


set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
add_definitions(-DRV1106_1103)


#Opencv 4
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}  
               ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/allocator/dma/dma_alloc.cpp
               ${CMAKE_CURRENT_SOURCE_DIR}/src/paho_c_pub.c
               ${CMAKE_CURRENT_SOURCE_DIR}/src/paho_c_sub.c
               ${CMAKE_CURRENT_SOURCE_DIR}/src/image_handle.cc
               ${CMAKE_CURRENT_SOURCE_DIR}/src/rtsp_opencv.cc
               ${CMAKE_CURRENT_SOURCE_DIR}/postprocess.cc               
               ${CMAKE_CURRENT_SOURCE_DIR}/yolov5.cc  
               )

add_compile_options(-g -Wall
                    -DISP_HW_V30 -DRKPLATFORM=ON -DARCH64=OFF
                    -DROCKIVA -DUAPI2
                    -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 
                    )

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath-link,${CMAKE_CURRENT_SOURCE_DIR}/lib:/usr/lib:/usr/arm-linux-gnueabihf/lib")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}  
                    ${OpenCV_LIBS}
                    rockiva
                    sample_comm
                    rockit
                    rockchip_mpp
                    rkaiq
                    pthread
                    m
                    dl
                    rtsp
                    rga
                    #md_share
                    crypto
                    ssl
                    paho-mqtt3a
                    paho-mqtt3cs
                    Threads::Threads
                    )


target_link_libraries(${PROJECT_NAME}     
                      ${LIBRKNNRT}
                     )

target_include_directories(${PROJECT_NAME} PRIVATE                        
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/utils
                           ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rknpu2/include
                           ${OpenCV_LIBS}
                          )
                    
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/luckfox_rtsp_yolov5_demo")
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/model/coco_80_labels_list.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/model)
file(GLOB RKNN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/model/*.rknn")
install(FILES ${RKNN_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/model)
                          
