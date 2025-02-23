
#ifndef _IMAGE_HANDLE_H_
#define _IMAGE_HANDLE_H_

#include "common.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>


#define RAW_IMAGE_WIDTH  1920
#define RAW_IMAGE_HEIGHT 1080
//#define USE_RGA_RESIZE 
#define USE_ASYNC_RKNN 1

#ifdef USE_RGA_RESIZE
#include "im2d_version.h"
#include "im2d_common.h"
#include "RgaUtils.h"
#include "im2d.hpp"
#include "dma_alloc.h"
#endif


#if 0
extern struct timeval time_debug;
#define pre_debug_time() do {\
    gettimeofday( &time_debug, NULL );\
	printf(" %d time: %3.4f \r\n", __LINE__ ,time_debug.tv_sec + time_debug.tv_usec*1e-6);\
}while(0)
#else
#define pre_debug_time() 
#endif


typedef struct {
    // disp size
    int width;
    int height;
    
    int srol_width;
    int srol_height;	
    // model size
    int model_width;
    int model_height;	
    float scale;
    int pollingarea;
    int pollingareacnt;
} det_manage_t;

typedef struct {
    
    int leftPadding;
    int topPadding;
    int target_s_x; //cat midpoint x
    int target_e_x; //cat midpoint x

    int region_sX;

	int sX;
    int sY;
    int eX;
    int eY; 

    int target_have;
}dect_ret_t;

cv::Mat letterbox(cv::Mat input, int iwidth, int iheight);

cv::Mat pollingdecetbox(cv::Mat input);

void mapCoordinates(int *x, int *y);

void draw_result(cv::Mat frame, void *od_results)/*8.2ms*/;

int get_model_hight();

int get_model_width();

int image_hanle_free();

int image_hanlde_init();
void *rknn_thread(void *arg);
int rkn_queue_add(cv::Mat frame);

extern int model_height;
extern int model_width;

#endif //_IMAGE_HANDLE_H_