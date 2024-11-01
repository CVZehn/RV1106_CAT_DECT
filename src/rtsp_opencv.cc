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


#include "rtsp_demo.h"
#include "sample_comm.h"
#include "yolov5.h"
//#include "move_detection.h"

#include "image_handle.h"
//#include <opencv2/imgcodecs.hpp>


#include "MQTTClient.h"
#include "pubsub_c_opts.h"

#define COMPILATION_DATE __DATE__ " " __TIME__

//#define USE_RGA_RESIZE 
#ifdef USE_RGA_RESIZE
#include "im2d_version.h"
#include "im2d_common.h"
#include "RgaUtils.h"
#include "im2d.hpp"
#include "dma_alloc.h"

rga_buffer_handle_t src_handle, dst_handle;
int src_buf_size, dst_buf_size;
int src_dma_fd, dst_dma_fd;
char *src_buf, *dst_buf;
#endif

#if 0
struct timeval t;;
#define pre_debug_time() do {\
    gettimeofday( &t, NULL );\
	printf(" %d time: %3.4f \r\n", __LINE__ ,t.tv_sec + t.tv_usec*1e-6);\
}while(0)
#else
#define pre_debug_time() 
#endif

static RK_S32 dm_venc_init(int chnId, int width, int height, RK_CODEC_ID_E enType) {
	printf("%s\n",__func__);
	VENC_RECV_PIC_PARAM_S stRecvParam;
	VENC_CHN_ATTR_S stAttr;
	memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));

	// RTSP H264	
	stAttr.stVencAttr.enType = enType;
	//stAttr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
	stAttr.stVencAttr.enPixelFormat = RK_FMT_RGB888;	
	stAttr.stVencAttr.u32Profile = 100;
	stAttr.stVencAttr.u32PicWidth = width;
	stAttr.stVencAttr.u32PicHeight = height;
	stAttr.stVencAttr.u32VirWidth = width;
	stAttr.stVencAttr.u32VirHeight = height;
	stAttr.stVencAttr.u32StreamBufCnt = 2;
	stAttr.stVencAttr.u32BufSize = width * height * 3 / 2;
	stAttr.stVencAttr.enMirror = MIRROR_NONE;
		
	stAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
	stAttr.stRcAttr.stH264Cbr.u32BitRate = 3 * 1024;
	stAttr.stRcAttr.stH264Cbr.u32Gop = 50;
	RK_MPI_VENC_CreateChn(chnId, &stAttr);

	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = -1;
	RK_MPI_VENC_StartRecvFrame(chnId, &stRecvParam);

	return 0;
}


// demo板dev默认都是0，根据不同的channel 来选择不同的vi节点
int vi_dev_init() {
	printf("%s\n", __func__);
	int ret = 0;
	int devId = 0;
	int pipeId = devId;

	VI_DEV_ATTR_S stDevAttr;
	VI_DEV_BIND_PIPE_S stBindPipe;
	memset(&stDevAttr, 0, sizeof(stDevAttr));
	memset(&stBindPipe, 0, sizeof(stBindPipe));
	// 0. get dev config status
	ret = RK_MPI_VI_GetDevAttr(devId, &stDevAttr);
	if (ret == RK_ERR_VI_NOT_CONFIG) {
		// 0-1.config dev
		ret = RK_MPI_VI_SetDevAttr(devId, &stDevAttr);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_SetDevAttr %x\n", ret);
			return -1;
		}
	} else {
		printf("RK_MPI_VI_SetDevAttr already\n");
	}
	// 1.get dev enable status
	ret = RK_MPI_VI_GetDevIsEnable(devId);
	if (ret != RK_SUCCESS) {
		// 1-2.enable dev
		ret = RK_MPI_VI_EnableDev(devId);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_EnableDev %x\n", ret);
			return -1;
		}
		// 1-3.bind dev/pipe
		stBindPipe.u32Num = pipeId;
		stBindPipe.PipeId[0] = pipeId;
		ret = RK_MPI_VI_SetDevBindPipe(devId, &stBindPipe);
		if (ret != RK_SUCCESS) {
			printf("RK_MPI_VI_SetDevBindPipe %x\n", ret);
			return -1;
		}
	} else {
		printf("RK_MPI_VI_EnableDev already\n");
	}

	return 0;
}

int vi_chn_init(int channelId, int width, int height) {
	int ret;
	int buf_cnt = 2;
	// VI init
	VI_CHN_ATTR_S vi_chn_attr;
	memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
	vi_chn_attr.stIspOpt.u32BufCount = buf_cnt;
	vi_chn_attr.stIspOpt.enMemoryType =
	    VI_V4L2_MEMORY_TYPE_DMABUF; // VI_V4L2_MEMORY_TYPE_MMAP;
	vi_chn_attr.stSize.u32Width = width;
	vi_chn_attr.stSize.u32Height = height;
	vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
	vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE; // COMPRESS_AFBC_16x16;
	vi_chn_attr.u32Depth = 2;
	ret = RK_MPI_VI_SetChnAttr(0, channelId, &vi_chn_attr);
	ret |= RK_MPI_VI_EnableChn(0, channelId);
	if (ret) {
		printf("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	return ret;
}

int dm_vpss_init(int VpssChn, int width, int height) {
	printf("%s\n",__func__);
	int s32Ret;
	VPSS_CHN_ATTR_S stVpssChnAttr;
	VPSS_GRP_ATTR_S stGrpVpssAttr;

	int s32Grp = 0;

	stGrpVpssAttr.u32MaxW = 4096;
	stGrpVpssAttr.u32MaxH = 4096;
	stGrpVpssAttr.enPixelFormat = RK_FMT_YUV420SP;
	stGrpVpssAttr.stFrameRate.s32SrcFrameRate = -1;
	stGrpVpssAttr.stFrameRate.s32DstFrameRate = -1;
	stGrpVpssAttr.enCompressMode = COMPRESS_MODE_NONE;

	stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
	stVpssChnAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
	stVpssChnAttr.enPixelFormat = RK_FMT_RGB888;
	stVpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
	stVpssChnAttr.stFrameRate.s32DstFrameRate = -1;
	stVpssChnAttr.u32Width = width;
	stVpssChnAttr.u32Height = height;
	stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;

	s32Ret = RK_MPI_VPSS_CreateGrp(s32Grp, &stGrpVpssAttr);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}

	s32Ret = RK_MPI_VPSS_SetChnAttr(s32Grp, VpssChn, &stVpssChnAttr);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}
	s32Ret = RK_MPI_VPSS_EnableChn(s32Grp, VpssChn);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}

	s32Ret = RK_MPI_VPSS_StartGrp(s32Grp);
	if (s32Ret != RK_SUCCESS) {
		return s32Ret;
	}
	return s32Ret;
}

#if 0
	static int a = 0;
	if(a == 40)
	{
		printf("-----------------debug pictue  resize.jpg!\n");
		cv::imwrite("resize.jpg",letterboxImage);
	}
	a++;
#endif

rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle g_rtsp_session;
static pthread_t venc_thread_0;
static void *rkipc_get_venc_0(void *arg);

int main(int argc, char *argv[]) {
	RK_S32 s32Ret = 0; 
		
	int height = RAW_IMAGE_HEIGHT;
	int width = RAW_IMAGE_WIDTH;
	int model_height = get_model_hight();
	int model_width = get_model_width();

	// Rknn model
	rknn_app_context_t rknn_app_ctx;	
	object_detect_result_list od_results;
    int ret;
	const char *model_path = "./model/yolov5.rknn";
    memset(&rknn_app_ctx, 0, sizeof(rknn_app_context_t));
    init_yolov5_model(model_path, &rknn_app_ctx);
	printf("init rknn model success!\n");
    init_post_process();

	//h264_frame	
	// VENC_STREAM_S stFrame;	
	// stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
 	VIDEO_FRAME_INFO_S h264_frame;
 	VIDEO_FRAME_INFO_S stVpssFrame;

	// rkaiq init
	RK_BOOL multi_sensor = RK_TRUE; //RK_TRUE	 RK_FALSE
	const char *iq_dir = "/etc/iqfiles";
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	// hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2; RK_AIQ_WORKING_MODE_NORMAL
	SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
	SAMPLE_COMM_ISP_Run(0);

	// rkmpi init
	if (RK_MPI_SYS_Init() != RK_SUCCESS) {
		RK_LOGE("rk mpi sys init fail!");
		return -1;
	}

	// rtsp init	
	g_rtsplive = create_rtsp_demo(554);
	g_rtsp_session = rtsp_new_session(g_rtsplive, "/live/0");
	rtsp_set_video(g_rtsp_session, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
	rtsp_sync_video_ts(g_rtsp_session, rtsp_get_reltime(), rtsp_get_ntptime());
	
	// vi init
	vi_dev_init();
	vi_chn_init(0, RAW_IMAGE_WIDTH, RAW_IMAGE_HEIGHT);

	// vpss init
	dm_vpss_init(0, RAW_IMAGE_WIDTH, RAW_IMAGE_HEIGHT);

	// bind vi to vpss
	MPP_CHN_S stSrcChn, stvpssChn;
	stSrcChn.enModId = RK_ID_VI;
	stSrcChn.s32DevId = 0;
	stSrcChn.s32ChnId = 0;

	stvpssChn.enModId = RK_ID_VPSS;
	stvpssChn.s32DevId = 0;
	stvpssChn.s32ChnId = 0;
	printf("====RK_MPI_SYS_Bind vi0 to vpss0====\n");
	s32Ret = RK_MPI_SYS_Bind(&stSrcChn, &stvpssChn);
	if (s32Ret != RK_SUCCESS) {
		RK_LOGE("bind 0 ch venc failed");
		return -1;
	}

	// venc init
	RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
	dm_venc_init(0, RAW_IMAGE_WIDTH, RAW_IMAGE_HEIGHT, enCodecType);

	mqtt_init();
	mqttsub_init();
	//struct md_ctx *md_ctx;
	//md_ctx = move_detection_init(width, height, 640, 640, 0);

	pthread_create(&venc_thread_0, NULL, rkipc_get_venc_0, NULL);
	
#ifdef USE_RGA_RESIZE
    src_buf_size = srol_width * srol_height * get_bpp_from_format(RK_FORMAT_RGB_888);
    dst_buf_size = model_width * model_height * get_bpp_from_format(RK_FORMAT_RGB_888);
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

	printf("venc init success\n");	
	int person_dectetime = 0;
	int fire_state = 0;
	int dect_demo = 0;
	printf("Compilation Date: %s\n", COMPILATION_DATE);

	while(1)
	{	
		// get vpss frame
		s32Ret = RK_MPI_VPSS_GetChnFrame(0,0, &stVpssFrame,-1); 											/*18.1ms*/
		if(s32Ret == RK_SUCCESS)
		{
			
			void *data = RK_MPI_MB_Handle2VirAddr(stVpssFrame.stVFrame.pMbBlk);								/*0.7ms*/
			
			//opencv	
			cv::Mat frame(height,width,CV_8UC3,data);	
			/*		
			//cv::Mat frame640;
        	//cv::resize(frame, frame640, cv::Size(640,640), 0, 0, cv::INTER_LINEAR);	
			//letterbox
			*/
			cv::Mat letterboxImage = pollingdecetbox(frame); 												/*104.4ms*/
			//cv::Mat letterboxImage = letterbox(frame, width, height);
			
			memcpy(rknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width*model_height*3); /*6.5ms*/
			inference_yolov5_model(&rknn_app_ctx, &od_results); 											/*97ms*/
			/*draw result*/
			draw_result(frame, &od_results);
			memcpy(data, frame.data, width * height * 3);	/*7.4ms*/
						
		}
		

		// send stream
		// encode H264
		RK_MPI_VENC_SendFrame(0, &stVpssFrame,-1);
		
		// release frame 
		s32Ret = RK_MPI_VPSS_ReleaseChnFrame(0, 0, &stVpssFrame);
		if (s32Ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_VI_ReleaseChnFrame fail %x", s32Ret);
		}
				
	}

	RK_MPI_SYS_UnBind(&stSrcChn, &stvpssChn);
	
	RK_MPI_VI_DisableChn(0, 0);
	RK_MPI_VI_DisableDev(0);
	
	RK_MPI_VPSS_StopGrp(0);
	RK_MPI_VPSS_DestroyGrp(0);
	
	RK_MPI_VENC_StopRecvFrame(0);
	RK_MPI_VENC_DestroyChn(0);


#ifdef USE_RGA_RESIZE  /*目前RGA 终止程序时会有buff无法释放的情况*/
release_buffer:
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    if (src_buf)
        free(src_buf);
    if (dst_buf)
        free(dst_buf);
#endif

	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);
	SAMPLE_COMM_ISP_Stop(0);

	RK_MPI_SYS_Exit();
	// Release rknn model
    release_yolov5_model(&rknn_app_ctx);		
    deinit_post_process();
	pthread_join(venc_thread_0, NULL);
	
	return 0;
}

static int g_video_run_ = 1;

static void *rkipc_get_venc_0(void *arg) {

	RK_LOGI("#Start %s thread, arg:%p\n", __func__, arg);
	VENC_STREAM_S stFrame;
	int loopCount = 0;
	int ret = 0;
	// FILE *fp = fopen("/data/venc.h265", "wb");
	stFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));

	while (g_video_run_) {

		// rtsp
		ret = RK_MPI_VENC_GetStream(0, &stFrame, -1);
		if(ret == RK_SUCCESS)
		{
			if(g_rtsplive && g_rtsp_session)
			{
				//printf("len = %d PTS = %d  %d\n",stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS, g_rtsplive);
				
				void *pData = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
				rtsp_tx_video(g_rtsp_session, (uint8_t *)pData, stFrame.pstPack->u32Len,
							  stFrame.pstPack->u64PTS);
				rtsp_do_event(g_rtsplive);
			}
		}
		
		ret = RK_MPI_VENC_ReleaseStream(0, &stFrame);
		if (ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", ret);
		}
	}
	if (stFrame.pstPack)
		free(stFrame.pstPack);
	// if (fp)
	// fclose(fp);

	return 0;
}