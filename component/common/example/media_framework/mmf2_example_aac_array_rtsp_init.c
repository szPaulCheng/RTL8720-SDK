 /******************************************************************************
	 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"

void mmf2_example_aac_array_rtsp_init(void)
{
	// Audio array input (AAC)
	array_t array;
	array.data_addr = (uint32_t) aac_sample;
	array.data_len = (uint32_t) aac_sample_size;
	array_ctx = mm_module_open(&array_module);
	if(array_ctx){
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_PARAMS, (int)&aac_array_params);
		mm_module_ctrl(array_ctx, CMD_ARRAY_SET_ARRAY, (int)&array);
		mm_module_ctrl(array_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(array_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(array_ctx, CMD_ARRAY_APPLY, 0);
		mm_module_ctrl(array_ctx, CMD_ARRAY_STREAMING, 1);	// streamming on
	}else{
		rt_printf("ARRAY open fail\n\r");
		goto mmf2_example_aac_array_rtsp_fail;
	}
	
	// RTSP
	rtsp2_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_ctx){
		mm_module_ctrl(rtsp2_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_aac_array_params);
		mm_module_ctrl(rtsp2_ctx, CMD_RTSP2_USE_RTP_TICK_INC, 1);	// RTP timstamp increase with fix value: 1024 
		mm_module_ctrl(rtsp2_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_example_aac_array_rtsp_fail;
	}
	rt_printf("RTSP2 opened\n\r");
	
	
	//--------------Link---------------------------
	siso_array_rtsp = siso_create();
	if(siso_array_rtsp){
		siso_ctrl(siso_array_rtsp, MMIC_CMD_ADD_INPUT, (uint32_t)array_ctx, 0);
		siso_ctrl(siso_array_rtsp, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_ctx, 0);
		siso_start(siso_array_rtsp);
	}else{
		rt_printf("siso_array_rtsp open fail\n\r");
		goto mmf2_example_aac_array_rtsp_fail;
	}
	rt_printf("siso_array_rtsp started\n\r");

	return;
mmf2_example_aac_array_rtsp_fail:
	
	return;
}