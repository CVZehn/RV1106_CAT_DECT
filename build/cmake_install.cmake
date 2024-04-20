# Install script for directory: /home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/zehn/luckfox_sdk/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5"
         RPATH "$ORIGIN/lib")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo" TYPE EXECUTABLE FILES "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/build/luckfox_rtsp_yolov5")
  if(EXISTS "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5"
         OLD_RPATH "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/3rdparty/rknpu2/Linux/armhf-uclibc:/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/lib:/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/media_out/lib:/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/mqttlib/openssl/lib:/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/mqttlib/paho/lib:"
         NEW_RPATH "$ORIGIN/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/home/zehn/luckfox_sdk/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-strip" "$ENV{DESTDIR}/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/luckfox_rtsp_yolov5")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/model/coco_80_labels_list.txt")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/model" TYPE FILE FILES "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/model/coco_80_labels_list.txt")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/model/yolov5.rknn")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/luckfox_rtsp_yolov5_demo/model" TYPE FILE FILES "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/model/yolov5.rknn")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/build/3rdparty.out/cmake_install.cmake")
  include("/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/build/utils.out/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/zehn/rv1106_rtso_yolov5/RV1106_CAT_DECT/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
