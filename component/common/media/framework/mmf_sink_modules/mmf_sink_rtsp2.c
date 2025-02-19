#include "mmf_sink.h"
#include "mmf_sink_rtsp2.h"

#define STREAM_FLOW_ID_BASE 0
static u32 stream_flow_id_bitmap = 0;
static _mutex stream_flow_id_bitmap_lock = NULL;

static int channel_idx=0;

// codec map
//
static int codec_map_v[] = {AV_CODEC_ID_MJPEG, AV_CODEC_ID_H264, AV_CODEC_ID_MP4V_ES};
static int codec_map_a[] = {AV_CODEC_ID_PCMU, AV_CODEC_ID_PCMA, AV_CODEC_ID_MP4A_LATM};

void rtsp2_mod_close(void* ctx)
{
	struct rtsp_context * rtsp_ctx = (struct rtsp_context *)ctx;
	
	for(int i=0;i<rtsp_ctx->nb_streams;i++){
		if(rtsp_ctx->stream_ctx[i].codec)
			free(rtsp_ctx->stream_ctx[i].codec);
		
		rtp_object_deinit(&rtsp_ctx->stream_ctx[i].rtpobj.payload);
	}

	rtsp_close(rtsp_ctx);
	rtsp_context_free(rtsp_ctx);
	if(stream_flow_id_bitmap_lock != NULL)
		rtw_mutex_free(&stream_flow_id_bitmap_lock);
	stream_flow_id_bitmap = 0;
}

void* rtsp2_mod_open(void)
{
	int timeout = 10;
	struct rtsp_context *rtsp_ctx = rtsp_context_create(2);
	if(!rtsp_ctx)	return NULL;
	
	/*open rtsp service task*/
	if(rtsp_open(rtsp_ctx)<0){
		rtsp_context_free(rtsp_ctx);
		rtsp_ctx = NULL;
		goto rtsp2_mod_open_fail;
	}
	
	while(!rtsp_is_service_enabled(rtsp_ctx)){
		rtw_mdelay_os(1000);
		if(timeout--<=0){
			printf("\n\rwait rtsp service time out...");
			goto rtsp2_mod_open_fail;
		}
	}
	
	// init payload object
	for(int i=0;i<rtsp_ctx->nb_streams;i++){
		rtp_object_init(&rtsp_ctx->stream_ctx[i].rtpobj.payload);
		rtsp_ctx->stream_ctx[i].codec = malloc(sizeof(struct codec_info));
		if(!rtsp_ctx->stream_ctx[i].codec)
			goto rtsp2_mod_open_fail;
		//rtsp_clock_init(rtsp_ctx->stream_ctx[i].codec->clock_rate);
	}
	return (void*)rtsp_ctx; 
	
rtsp2_mod_open_fail:
	printf("[ERROR] rtsp2_mod_open Fail\n\r");
	//rtp_object_deinit(&rtp_payload);
	rtsp2_mod_close(rtsp_ctx);
	return NULL;
}

// to get stream id,
// need mutex to protect this routine? i dont think it is necessary
int rtsp2_mod_set_param(void* ctx, int cmd, int arg)
{
	struct rtsp_context *rtsp_ctx = (struct rtsp_context *)ctx;
	struct stream_context *stream_ctx = &rtsp_ctx->stream_ctx[channel_idx];
	int *codec_map = NULL;
	
	switch(cmd){
		case CMD_SET_STREAMMING:
			if(arg == ON){
				rtsp_start(rtsp_ctx);
				// insert to input queue
				for(int i=0;i<rtsp_ctx->nb_streams;i++)
					rtp_object_in_stream_queue(&rtsp_ctx->stream_ctx[i].rtpobj.payload, &rtsp_ctx->stream_ctx[i]); 
			}else
				rtsp_stop(rtsp_ctx);
			break;
		case CMD_RSTP2_SELECT_CHANNEL:
			channel_idx = arg;
			break;
		case CMD_RSTP2_SET_FRAMERATE:
			stream_ctx->framerate = arg;
			break;
		case CMD_RSTP2_SET_BITRATE:
			stream_ctx->bitrate = arg;
			break;
		case CMD_RSTP2_SET_SAMPLERATE:
			stream_ctx->samplerate = arg;
			break;
		case CMD_RSTP2_SET_CHANNEL:
			stream_ctx->channel = arg;
			break;
		// for H264
		case CMD_RSTP2_SET_SPS:
			set_sps_string((char*)arg);
			break;
		case CMD_RSTP2_SET_PPS:
			set_pps_string((char*)arg);
			break;
		case CMD_RSTP2_SET_LEVEL:
			set_profile_lv_string((char*)arg);
			break;
		case CMD_RSTP2_SET_APPLY:
			if(stream_flow_id_bitmap_lock == NULL)
				rtw_mutex_init(&stream_flow_id_bitmap_lock);
			if(stream_ctx->stream_id < 0)
				stream_ctx->stream_id = rtsp_get_number(STREAM_FLOW_ID_BASE, &stream_flow_id_bitmap, &stream_flow_id_bitmap_lock);
			rtp_stream_statistics_sync(stream_ctx);
			stream_ctx->rtpobj.codec_id = stream_ctx->codec->codec_id;
			break;
		case CMD_RSTP2_SET_CODEC:
			codec_map = ((arg&0xf0)==0x00)?codec_map_v:codec_map_a;
			get_codec_by_id(stream_ctx->codec, codec_map[arg&0xf]);
			stream_ctx->media_type = rtsp_parse_stream_media_type(stream_ctx->codec);
			rtp_load_o_handler_by_codec_id(&stream_ctx->rtpobj.payload, codec_map[arg&0xf]);
			if(stream_ctx->rtpobj.payload.rtp_object_handler == NULL)
				return -EINVAL;
			stream_ctx->rtp_timestamp = xTaskGetTickCount();
			break;
		case CMD_RSTP2_SET_FLAG:
			if(arg == TIME_SYNC_DIS)
				time_sync_disable();
			else if(arg == TIME_SYNC_EN)
				time_sync_enable();
			break;
		case CMD_RSTP2_SET_START_CB:
			rtsp_ctx->cb_start = (int (*)(void*))arg;
			break;
		case CMD_RSTP2_SET_STOP_CB:
			rtsp_ctx->cb_stop = (int (*)(void*))arg;
			break;
		case CMD_RSTP2_SET_PAUSE_CB:
			rtsp_ctx->cb_pause = (int (*)(void*))arg;
			break;
		case CMD_RSTP2_SET_CUSTOM_CB:
			rtsp_ctx->cb_custom = (int (*)(void*))arg;
			break;
		default:
			break;
	}
	return 0;
}

// private function
int rtsp2_mod_wait_complete(void)
{
	return 1;
}

// private function
int rtsp2_mod_compelete_cb(void)
{
	return 1;
}


int rtsp2_mod_handle(void* ctx, void* b)
{
	struct rtsp_context *rtsp_ctx = (struct rtsp_context *)ctx;
	exch_buf_t *exbuf = (exch_buf_t*)b;
	
	struct stream_context *stream_ctx = NULL;
	struct rtp_object *payload = NULL;//&rtp_payload;
	
	if(exbuf->state != STAT_READY)
		return -EAGAIN;
	
	// get channel
	int *codec_map = ((exbuf->codec_fmt&0xf0)==0x00)?codec_map_v:codec_map_a;
	for(int i=0;i<rtsp_ctx->nb_streams;i++){
		if(rtsp_ctx->stream_ctx[i].rtpobj.codec_id == codec_map[exbuf->codec_fmt&0xf]){
			stream_ctx = &rtsp_ctx->stream_ctx[i];
			break;
		}
	}
	
	if(stream_ctx == NULL) {
		exbuf->state = STAT_INIT;
		printf("stream_ctx= NULL!\n\r");
		return -EFAULT;
	}

	if(rtsp_ctx->state != RTSP_PLAYING)
		goto end;
	
	// wait output not empty and get one
	// Get payload from rtsp module
	do{
		payload = rtp_object_out_stream_queue(stream_ctx);
		if(payload==NULL)	vTaskDelay(1);
	}while(payload == NULL);
	
	// insert payload to rtsp_ctx stream
	//printf("%d\n\r", xTaskGetTickCount());
	/*fill payload*/
	payload->index = exbuf->index;
	payload->data = exbuf->data;
	payload->len = exbuf->len;
	//payload->timestamp = (rtw_get_current_time()-rtsp_tick_offset)*90;
	payload->timestamp = exbuf->timestamp;//rtsp_get_current_tick();
	if(payload->timestamp==0) {
		payload->timestamp = xTaskGetTickCount();
	}
	else
		payload->timestamp = exbuf->timestamp;
	
	//printf("ts: %8x\n\r", payload->timestamp);
	/* because we will fill&send a complete frame in single rtp object, set both fs & fe to 1 and fd to 0*/
	rtp_object_set_fs(payload, 1);
	rtp_object_set_fe(payload, 1);
	rtp_object_set_fd(payload, 0);
	/* set payload state to READY to enable rtp task process this rtp object;*/
	payload->state = RTP_OBJECT_READY;
	
	rtp_object_in_stream_queue(payload, stream_ctx);
	
	// TODO: use inter task communication
	// wait payload state to IDLE or USED
	rtsp2_mod_wait_complete();
	while(list_empty(&stream_ctx->output_queue))
		vTaskDelay(1);
end:
	exbuf->state = STAT_USED;
	return 0;
}

msink_module_t rtsp2_module = 
{
	.create = rtsp2_mod_open,
	.destroy = rtsp2_mod_close,
	.set_param = rtsp2_mod_set_param,
	.handle = rtsp2_mod_handle,
};

msink_module_t rtsp2_module2 = 
{
	.create = rtsp2_mod_open,
	.destroy = rtsp2_mod_close,
	.set_param = rtsp2_mod_set_param,
	.handle = rtsp2_mod_handle,
};

msink_module_t rtsp2_module3 = 
{
	.create = rtsp2_mod_open,
	.destroy = rtsp2_mod_close,
	.set_param = rtsp2_mod_set_param,
	.handle = rtsp2_mod_handle,
};

