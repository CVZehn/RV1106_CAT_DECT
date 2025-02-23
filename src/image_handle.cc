
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

struct timeval time_debug;

#ifdef USE_RGA_RESIZE
rga_buffer_handle_t src_handle, dst_handle;
int src_buf_size, dst_buf_size;
int src_dma_fd, dst_dma_fd;
char *src_buf, *dst_buf;
#endif

det_manage_t detect_manage = {
	.width    = RAW_IMAGE_WIDTH,
    .height   = RAW_IMAGE_HEIGHT,
    .srol_width = 1080,
    .srol_height = 1080,	
    .model_width = 640,
    .model_height = 640,	
	
 	.pollingarea = 0,
 	.pollingareacnt = 3,
};

dect_ret_t detect_result = {
    .target_s_x = 0, //cat midpoint x
    .target_e_x = 0, //cat midpoint x
 	.region_sX = 0,
};

int pos_x_dect[3] = {0, 420, 840};
int pos_mid_dect[2] = {750, 1170};

object_detect_result_list od_results[2];
rknn_app_context_t rknn_app_ctx;	
std::vector<cv::Mat> resizeImages(2); // 创建一个包含两个cv::Mat对象的向量

uint8_t resize_ok[2] = {0,0};
uint8_t rkndo_ok[2] = {0,0};
int model_height;
int model_width;


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

	ret = imcheck(src, dst, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
    }

    ret = imresize(src, dst);
    if (ret == IM_STATUS_SUCCESS) {

        // printf(" running success!\n");
		cv::Mat letterboxImage = cv::Mat(cv::Size(model_width, model_height), CV_8UC3, dst_buf);
		return letterboxImage; 	
    } else {
        printf(" running failed, %s\n", imStrError((IM_STATUS)ret));
    	cv::resize(input, inputScale, cv::Size(inputWidth,inputHeight), 0, 0, cv::INTER_LINEAR);  //这个地方耗时80ms  需要优化
		cv::Mat letterboxImage(640, 640, CV_8UC3,cv::Scalar(0, 0, 0));
    	cv::Rect roi(detect_result.leftPadding, detect_result.topPadding, inputWidth, inputHeight);
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
/*1.三个区域检测(1080 *1080) 检测到猫猫进行第二步*/
/*2. 根据猫猫的位置选择检测哪个区域*/
/*3.（2.）的三区域检测检测不到则回到第一步*/
cv::Mat pollingdecetbox(cv::Mat input)
{
#if 1
#else
	detect_manage.pollingarea = (detect_manage.pollingarea + 0) % detect_manage.pollingareacnt;
	detect_result.region_sX = (1920 - 1080) * detect_manage.pollingarea;
#endif
	assert(detect_result.region_sX <= 840);
	detect_result.region_sX = 420; /* todo */
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
	uint8_t have_cat = 0;
	uint32_t have_target = 0;
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

			detect_result.sX = (int)(det_result->box.left);	
			detect_result.sY = (int)(det_result->box.top);	
			detect_result.eX = (int)(det_result->box.right);	
			detect_result.eY = (int)(det_result->box.bottom);

			mapCoordinates(&detect_result.sX,&detect_result.sY);
			mapCoordinates(&detect_result.eX,&detect_result.eY);

			printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
					 detect_result.sX, detect_result.sY, detect_result.eX, detect_result.eY, det_result->prop);

		#if DRAW_LINE
			cv::rectangle(frame,cv::Point(detect_result.sX ,detect_result.sY),
						        cv::Point(detect_result.eX ,detect_result.eY),
								cv::Scalar(0,255,0),3);
			sprintf(text, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
			cv::putText(frame,text,cv::Point(detect_result.sX, detect_result.sY - 8),
										 cv::FONT_HERSHEY_SIMPLEX,1,
										 cv::Scalar(0,255,0),2);

		#endif
			if(det_result->cls_id == 15 || det_result->cls_id == 16)
			{
				cat_dected = 1;
				have_cat = 1;
				have_target = 1;
				detect_result.target_s_x = detect_result.sX;
				detect_result.target_e_x = detect_result.eX;
    			gettimeofday(&t, NULL );
				if(t.tv_usec - usecond_le >= 300*1000 && Get_TeasingSW())
				{
					mqtt_guard_ps_set(detect_result.sX,detect_result.sY,detect_result.eX,detect_result.eY);
					usecond_le = t.tv_usec;
				}
			}
			if(det_result->cls_id == 0 && have_cat == 0) {
				have_target = 1;
				detect_result.target_s_x = detect_result.sX;
				detect_result.target_e_x = detect_result.eX;
			}
			
		}
	}
	memset(text,0,8);
	if(detect_result.target_have == 0)
	{
		assert(detect_manage.pollingarea <= 2);
		detect_result.region_sX = pos_x_dect[detect_manage.pollingarea];
		detect_manage.pollingarea = (detect_manage.pollingarea + 1) % detect_manage.pollingareacnt;
	}
	else 
	{
		uint32_t mid_pos = (detect_result.target_e_x - detect_result.target_s_x) / 2 + detect_result.target_s_x;
		printf("mid pos %d\n", mid_pos);
		if (mid_pos < pos_mid_dect[0]) {
			detect_result.region_sX = pos_x_dect[0];
		} else if (mid_pos < pos_mid_dect[1]) {
			detect_result.region_sX = pos_x_dect[1];
		} else {
			detect_result.region_sX = pos_x_dect[2];
		}
	}
	detect_result.target_have = have_target;
}

void dm_cat_is_dected() 
{
	if(cat_dected == 0)
	{
		detect_result.target_s_x = 0;
		detect_result.target_e_x = 0;
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

int image_hanlde_init()
{
	detect_result.region_sX = 0;
	detect_manage.pollingarea = 0;
#ifdef USE_RGA_RESIZE
	int ret;
	src_buf_size = detect_manage.srol_width * detect_manage.srol_height * get_bpp_from_format(RK_FORMAT_RGB_888);
	dst_buf_size = detect_manage.model_width * detect_manage.model_height * get_bpp_from_format(RK_FORMAT_RGB_888);
	printf("mem size :%x %x\n", src_buf_size, dst_buf_size);

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
#endif 
}

int image_hanle_free()
{
#ifdef USE_RGA_RESIZE  /*目前RGA 终止程序时会有buff无法释放的情况*/
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (src_buf)
        free(src_buf);
    if (dst_buf)
        free(dst_buf);
#endif
}

void *rknn_thread(void *arg)
{
	uint8_t idx = 0xff;
	while (1)
	{
		for (uint8_t i = 0; i < 2; i++) {
			if (resize_ok[i]) {
				idx = i;
				break;
			}
		}
		if (idx != 0xff) {
			cv::Mat temp_image = resizeImages.at(idx);
			memcpy(rknn_app_ctx.input_mems[0]->virt_addr, temp_image.data, model_width*model_height*3); /*6.5ms*/
			inference_yolov5_model(&rknn_app_ctx, &od_results[idx]);
			resize_ok[idx] = 0;
			rkndo_ok[idx] = 1;
		}
	}
	return 0;
}

int rkn_queue_add(cv::Mat frame)
{
	for (uint8_t i = 0; i < 2; i++) {
		if (resize_ok[i] == 0) {
			resizeImages.at(i) = frame;
			resize_ok[i] = 1;
			return 0;
		}
	}
	return -1;
}

int rkn_result_get(object_detect_result_list *result)
{
	for (uint8_t i = 0; i < 2; i++) {
		if (rkndo_ok[i] == 1) {
			memcpy(result, &od_results[i], sizeof(object_detect_result_list));
			resize_ok[i] = 0;
			return 0;
		}
	}
	return -1;
}