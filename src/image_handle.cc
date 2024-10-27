
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <vector>


#include "image_handle.h"
#include "rtsp_demo.h"
#include "sample_comm.h"
#include "yolov5.h"

#include "MQTTClient.h"
#include "pubsub_c_opts.h"
//#include "move_detection.h"

det_manage_t detect_manage = {
	.width    = RAW_IMAGE_WIDTH,
    .height   = RAW_IMAGE_HEIGHT,
    .srol_width = 1080,
    .srol_height = 1080,	
    .model_width = 640,
    .model_height = 640,	
	
 	.pollingarea = 0,
 	.pollingareacnt = 2,
};

dect_ret_t detect_result = {
    .cat_midpoint_x = 0, //cat midpoint x
    .cat_midpoint_y = 0, //cat midpoint x
 	.region_sX = 0,
};

cv::Mat letterbox(cv::Mat input, int iwidth, int iheight)
{
	float model_width = (float)detect_manage.model_width;
	float model_height = (float)detect_manage.model_height;

	float scaleX = model_width  / (float)iwidth; //0.888
	float scaleY = model_height / (float)iheight; //1.125	
	detect_manage.scale = scaleX < scaleY ? scaleX : scaleY;
	
	int inputWidth   = (int)((float)iwidth * detect_manage.scale);
	int inputHeight  = (int)((float)iheight * detect_manage.scale);

	detect_result.leftPadding = (model_width  - inputWidth) / 2;
	detect_result.topPadding  = (model_height - inputHeight) / 2;	
	
	cv::Mat inputScale;

#ifdef USE_RGA_RESIZE
	int ret;
    rga_buffer_t src = {};
    rga_buffer_t dst = {};
    im_rect src_rect = {};
    im_rect dst_rect = {};

    pre_debug_time();
	
	cv::Mat tempImage(iwidth, iheight, CV_8UC3,cv::Scalar(0, 0, 0));
	input.copyTo(tempImage);
    memcpy(src_buf, tempImage.data, iwidth * iheight * get_bpp_from_format(RK_FORMAT_RGB_888));
    memset(dst_buf, 0, dst_buf_size);
	
    src_handle = importbuffer_fd(src_dma_fd, iwidth * iheight * get_bpp_from_format(RK_FORMAT_RGB_888));
    dst_handle = importbuffer_fd(dst_dma_fd, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0) {
        printf("import dma_fd error!\n");
		return inputScale;
    }

    src = wrapbuffer_handle(src_handle, iwidth, iheight, RK_FORMAT_RGB_888);
    dst = wrapbuffer_handle(dst_handle, model_width, model_height, RK_FORMAT_RGB_888);

    pre_debug_time();
	ret = imcheck(src, dst, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
    }

    ret = imresize(src, dst);
    if (ret == IM_STATUS_SUCCESS) {

        // printf(" running success!\n");
		cv::Mat letterboxImage = cv::Mat(cv::Size(model_width, model_height), CV_8UC3, dst_buf);
        pre_debug_time();
		return letterboxImage; 	
    } else {
        printf(" running failed, %s\n", imStrError((IM_STATUS)ret));
    	cv::resize(input, inputScale, cv::Size(inputWidth,inputHeight), 0, 0, cv::INTER_LINEAR);  //这个地方耗时80ms  需要优化
		cv::Mat letterboxImage(640, 640, CV_8UC3,cv::Scalar(0, 0, 0));
    	cv::Rect roi(leftPadding, topPadding, inputWidth, inputHeight);
    	inputScale.copyTo(letterboxImage(roi));
		return letterboxImage; 	
    }
#else
    cv::resize(input, inputScale, cv::Size(inputWidth,inputHeight), 0, 0, cv::INTER_LINEAR);  //这个地方耗时80ms  需要优化
	cv::Mat letterboxImage(640, 640, CV_8UC3,cv::Scalar(0, 0, 0));
    cv::Rect roi(detect_result.leftPadding, detect_result.topPadding, inputWidth, inputHeight);
    inputScale.copyTo(letterboxImage(roi));
	return letterboxImage; 	
#endif


}
/*1.两个区域检测(1080 *1080) 检测到猫猫进行第二步*/
/*2. 根据猫猫的位置选择检测哪个区域*/
/*3.（2.）的三区域检测检测不到则回到第一步*/
cv::Mat pollingdecetbox(cv::Mat input)
{
#if 0
	if(cat_midpoint_x == 0 && cat_midpoint_y == 0)
	{
		detect_manage.pollingarea = (detect_manage.pollingarea + 1) % detect_manage.pollingareacnt;
		detect_manage.region_sX = (1920 - 1080) * detect_manage.pollingarea;
	}
	else 
	{
		detect_manage.region_sX = (cat_midpoint_x - 1080 /2) < 0 ? 0 : (cat_midpoint_x - 1080 /2);
		detect_manage.region_sX = (cat_midpoint_x - 1080 /2) > 840 ? 840 : (cat_midpoint_x - 1080 /2);
	}
#else
	detect_manage.pollingarea = (detect_manage.pollingarea + 1) % detect_manage.pollingareacnt;
	detect_result.region_sX = (1920 - 1080) * detect_manage.pollingarea;
#endif
	// printf("area %d!\n", detect_manage.pollingarea);
	#if 0
	if(detect_manage.pollingarea == 0)
	{
		return letterbox(input, width, height);
	}
	else
	#endif 

	{
		//cv::Mat letterboxImage(960, 1080, CV_8UC3,cv::Scalar(0, 0, 0));
    	cv::Rect region_of_interest(detect_result.region_sX, 0, detect_manage.srol_width, detect_manage.srol_height);
		cv::Mat letterboxImage = input(region_of_interest);
		return letterbox(letterboxImage, detect_manage.srol_width ,detect_manage.srol_height);
	}
}

void mapCoordinates(int *x, int *y) 
{	
	int ap = detect_result.region_sX;
	int mx = *x - detect_result.leftPadding;
	int my = *y - detect_result.topPadding;

    *x = (int)((float)mx / detect_manage.scale) + ap;
    *y = (int)((float)my / detect_manage.scale); 
}

u32 cat_dected = 0;
struct timeval t;
u32 usecond_le = 0;

void draw_result(cv::Mat frame, void *results)/*8.2ms*/
{
	object_detect_result_list *od_results = (object_detect_result_list *)results;
	char text[16];
	for(int i = 0; i < od_results->count; i++)                                                       
	{	
		//获取框的四个坐标 
		if(od_results->count >= 1)
		{
			object_detect_result *det_result = &(od_results->results[i]);				
			if (det_result->cls_id != 0 && det_result->cls_id != 15 && det_result->cls_id != 16)
			{
			  continue;
			}

			detect_result.sX = (int)(det_result->box.left   );	
			detect_result.sY = (int)(det_result->box.top 	  );	
			detect_result.eX = (int)(det_result->box.right  );	
			detect_result.eY = (int)(det_result->box.bottom );

			mapCoordinates(&detect_result.sX,&detect_result.sY);
			mapCoordinates(&detect_result.eX,&detect_result.eY);

			printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
					 detect_result.sX, detect_result.sY, detect_result.eX, detect_result.eY, det_result->prop);

			cv::rectangle(frame,cv::Point(detect_result.sX ,detect_result.sY),
						        cv::Point(detect_result.eX ,detect_result.eY),
								cv::Scalar(0,255,0),3);
			sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
			cv::putText(frame,text,cv::Point(detect_result.sX, detect_result.sY - 8),
										 cv::FONT_HERSHEY_SIMPLEX,1,
										 cv::Scalar(0,255,0),2);

			if(det_result->cls_id == 15 || det_result->cls_id == 16)
			{
				cat_dected = 1;
				detect_result.cat_midpoint_x = (detect_result.eX - detect_result.sX) / 2 + detect_result.sX;
				detect_result.cat_midpoint_y = (detect_result.eY - detect_result.sY) / 2 + detect_result.sY;
    			gettimeofday(&t, NULL );
				if(t.tv_usec - usecond_le >= 300*1000 && Get_TeasingSW())
				{
					mqtt_guard_ps_set(detect_result.sX,detect_result.sY,detect_result.eX,detect_result.eY);
					usecond_le = t.tv_usec;
				}

			} 
			
		}
	}
	memset(text,0,8);
}

void dm_cat_is_dected() 
{
	if(cat_dected == 0)
	{
		detect_result.cat_midpoint_x = 0;
		detect_result.cat_midpoint_y = 0;
    	gettimeofday(&t, NULL );
		if(t.tv_usec - usecond_le >= 300*1000 && Get_TeasingSW())
		{
			mqtt_guard_ps_set(840,detect_result.sY,detect_result.eX,detect_result.eY);
			usecond_le = t.tv_usec;
		}
		
	}
}

int get_model_hight()
{
	return detect_manage.model_height;
}

int get_model_width()
{
	return detect_manage.model_width;
}