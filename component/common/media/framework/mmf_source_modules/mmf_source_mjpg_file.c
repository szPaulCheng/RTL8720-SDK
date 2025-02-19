#include "mmf_source.h"
#include "mmf_source_mjpg_file.h"

void* mjpgf_mod_open(void)
{
	return (void*)1;
}

void mjpgf_mod_close(void* ctx)
{
	(void)ctx;
	return;
}

int mjpgf_mod_set_param(void* ctx, int cmd, int arg)
{
	switch(cmd){
		default:
			break;
	}
	return 0;
}

	
static int cnt = 0;
int mjpgf_mod_handle(void* ctx, void* b)
{
	//printf("[src in] ");
	exch_buf_t *exbuf = (exch_buf_t*)b;
	
	/*get uvc buffer for new data*/
	if(exbuf->state!=STAT_READY){	
		if(cnt++&0x1){
			exbuf->data = (unsigned char *)PIC_320x240_2;
			exbuf->len = PIC_LEN_2;
		}else{
			exbuf->data = (unsigned char *)PIC_320x240_1;
			exbuf->len = PIC_LEN_1;
		}
		
		exbuf->index = 0;
		exbuf->timestamp = 0;
		//vTaskDelay(500);
		exbuf->state = STAT_READY;
		
		mmf_source_add_exbuf_sending_list_all(exbuf);
		//printf("[src ret] ");
	}
	return 0;

}

msrc_module_t mjpgf_module =
{
	.create = mjpgf_mod_open,
	.destroy = mjpgf_mod_close,
	.set_param = mjpgf_mod_set_param,
	.handle = mjpgf_mod_handle,
};
