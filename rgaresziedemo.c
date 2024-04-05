/*
 * Copyright (C) 2022  Rockchip Electronics Co., Ltd.
 * Authors:
 *     YuQiaowei <cerf.yu@rock-chips.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "rga_resize_demo"
#include "iostream"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <linux/stddef.h>
#include <string.h>
#include "RgaUtils.h"
#include "im2d.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "utils.h"
#include "dma_alloc.h"
#define LOCAL_FILE_PATH "/data"

int main() {
    int ret = 0;
    int src_width, src_height, src_format;
    int dst_width, dst_height, dst_format;
    char *src_buf, *dst_buf;
    int src_buf_size, dst_buf_size;
    int src_dma_fd, dst_dma_fd;
    rga_buffer_t src = {};
    rga_buffer_t dst = {};
    im_rect src_rect = {};
    im_rect dst_rect = {};
    rga_buffer_handle_t src_handle, dst_handle;
    int64_t ts;

//    memset(&src_img, 0, sizeof(src_img));
//    memset(&dst_img, 0, sizeof(dst_img));

    cv::Mat image_src =cv::imread("000026.jpg");


    src_width = image_src.cols;
    src_height = image_src.rows;
    std::cout <<"src_width=" <<src_width <<std::endl;
    src_format = RK_FORMAT_RGB_888;

    dst_width = 320;
    dst_height =320;
    dst_format = RK_FORMAT_RGB_888;
    std::cout <<"src_width=" <<src_width <<std::endl;

    cv::Mat resize_img;
    ts = get_cur_us();
    cv::resize(image_src,resize_img,cv::Size(320,320));
    printf("%s OPENCV RESIZE! cost %ld us\n", LOG_TAG, get_cur_us() - ts);


    src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
    dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

//    src_buf = (char *)malloc(src_buf_size);
//    dst_buf = (char *)malloc(dst_buf_size);

    /* Allocate dma_buf from CMA, return dma_fd and virtual address */
    ret = dma_buf_alloc(RV1106_CMA_HEAP_PATH, src_buf_size, &src_dma_fd, (void **)&src_buf);
    if (ret < 0) {
        printf("alloc src CMA buffer failed!\n");
        return -1;
    }

    ret = dma_buf_alloc(RV1106_CMA_HEAP_PATH, dst_buf_size, &dst_dma_fd, (void **)&dst_buf);
    if (ret < 0) {
        printf("alloc dst CMA buffer failed!\n");
        dma_buf_free(src_buf_size, &src_dma_fd, src_buf);
        return -1;
    }

  // memset(&src, 0, sizeof(src));

  // src_buf =(char *)image_src.data;
      memcpy(src_buf, image_src.data,src_buf_size);
  //  src = wrapbuffer_virtualaddr((void*)image_src.data, src_width, src_height, RK_FORMAT_RGB_888);

//    /* fill image data */
//    if (0 != read_image_from_file(src_buf, LOCAL_FILE_PATH, src_width, src_height, src_format, 0)) {
//        printf("src image read err\n");
//        draw_rgba(src_buf, src_width, src_height);
//      }
 //  memset(dst_buf, 0, dst_buf_size);
    memset(dst_buf, 0x33, dst_buf_size);

    /*
     * Import the allocated dma_fd into RGA by calling
     * importbuffer_fd, and use the returned buffer_handle
     * to call RGA to process the image.
     */
    src_handle = importbuffer_fd(src_dma_fd, src_buf_size);
    dst_handle = importbuffer_fd(dst_dma_fd, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0) {
        printf("import dma_fd error!\n");
        ret = -1;
       // goto free_buf;
    }

    src = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
    dst = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);

    /*
     * Scale up the src image to 1920*1080.
        --------------    ---------------------
        |            |    |                   |
        |  src_img   |    |     dst_img       |
        |            | => |                   |
        --------------    |                   |
                          |                   |
                          ---------------------
     */

    ret = imcheck(src, dst,src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        return -1;
    }

//    ret = imcopy(src, dst);
//    if (ret == IM_STATUS_SUCCESS) {
        printf("%s running success! cost %ld us\n", LOG_TAG, get_cur_us() - ts);
//    } else {
//        printf("%s running failed, %s\n", LOG_TAG, imStrError((IM_STATUS)ret));
//       // goto release_buffer;
//    }


     ts = get_cur_us();

    ret = imresize(src, dst);
    if (ret == IM_STATUS_SUCCESS) {
        printf("%s running success!\n", LOG_TAG);
         printf("%s RGA RESIZE! cost %ld us\n", LOG_TAG, get_cur_us() - ts);
    } else {
        printf("%s running failed, %s\n", LOG_TAG, imStrError((IM_STATUS)ret));
       // goto release_buffer;
    }

    cv::Mat _img = cv::Mat(cv::Size(dst_width, dst_height), CV_8UC3, dst_buf);
   // cv::Mat _img2 = cv::Mat(cv::Size(src_width, src_height), CV_8UC3, src_buf);
    cv::imwrite("resize.jpg",_img);

    write_image_to_file(dst_buf, LOCAL_FILE_PATH, dst_width, dst_height, dst_format, 0);

release_buffer:
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (src_buf)
        free(src_buf);
    if (dst_buf)
        free(dst_buf);

    return ret;
}

