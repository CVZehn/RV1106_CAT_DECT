
#ifndef _IMAGE_HANDLE_H_
#define _IMAGE_HANDLE_H_

#include "common.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>


#define RAW_IMAGE_WIDTH  1920
#define RAW_IMAGE_HEIGHT 1080

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
    int cat_midpoint_x; //cat midpoint x
    int cat_midpoint_y; //cat midpoint x

    int region_sX;

	int sX;
    int sY;
    int eX;
    int eY; 
}dect_ret_t;


cv::Mat letterbox(cv::Mat input, int iwidth, int iheight);

cv::Mat pollingdecetbox(cv::Mat input);

void mapCoordinates(int *x, int *y);

void draw_result(cv::Mat frame, void *od_results)/*8.2ms*/;

int get_model_hight();

int get_model_width();

#endif //_IMAGE_HANDLE_H_