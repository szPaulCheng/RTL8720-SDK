/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *  Copyright (c) 2016 Daniel Pirch.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "fvad.h"

#include <stdlib.h>
#include "vad_core.h"

// valid sample rates in kHz
static const int valid_rates[] = { 8, 16, 32, 48 };

// VAD process functions for each valid sample rate
static int (*const process_funcs[])(VadInstT *, const int16_t *, size_t) = {
    WebRtcVad_CalcVad8khz,
    WebRtcVad_CalcVad16khz,
    WebRtcVad_CalcVad32khz,
    WebRtcVad_CalcVad48khz,
};

// valid frame lengths in ms
static const int valid_frame_times[] = { 10, 20, 30 };


struct Fvad {
    VadInstT core;
    int rate_idx; // index in valid_rates and process_funcs arrays
};


Fvad *fvad_new()
{
    Fvad *inst = malloc(sizeof * inst);
    //printf("vad = %d\n", sizeof * inst);

    if (inst) {
        fvad_reset(inst);
    }

    return inst;
}


void fvad_free(Fvad *inst)
{
    assert(inst);
    free(inst);
}


void fvad_reset(Fvad *inst)
{
    assert(inst);

    int rv = WebRtcVad_InitCore(&inst->core);
    assert(rv == 0);
    inst->rate_idx = 0;
}


int fvad_set_mode(Fvad *inst, int mode)
{
    assert(inst);
    int rv = WebRtcVad_set_mode_core(&inst->core, mode);
    assert(rv == 0 || rv == -1);
    return rv;
}


int fvad_set_sample_rate(Fvad *inst, int sample_rate)
{
    assert(inst);

    for (int i = 0; i < arraysize(valid_rates); i++) {
        if (valid_rates[i] * 1000 == sample_rate) {
            inst->rate_idx = i;
            return 0;
        }
    }

    return -1;
}


static bool valid_length(int rate_idx, size_t length)
{
    int samples_per_ms = valid_rates[rate_idx];

    for (int i = 0; i < arraysize(valid_frame_times); i++) {
        if (valid_frame_times[i] * samples_per_ms == length) {
            return true;
        }
    }

    return false;
}

//FILE * fp_tai = NULL;
int rv_old = 0;
int fvad_process(Fvad *inst, const int16_t *frame, size_t length)
{
    assert(inst);

    if (!valid_length(inst->rate_idx, length)) {
        return -1;
    }

#if 0

    if (fp_tai == NULL) {
        fp_tai = fopen("/sdcard/record_tai.pcm", "wb+");

        if (fp_tai == NULL) {
            printf("fail fopen /sdcard/record_tai.pcm\n");
        }
    }

    if (fp_tai) {
        static int fwrite_count = 0;
        int ret_fwrite = fwrite(frame, 1, length, fp_tai);
        fwrite_count ++;
        printf("frame %p length %d ret_fwrite %d\n", frame, length, ret_fwrite);

        if (fwrite_count > 400) {
            fclose(fp_tai);
            printf("fclose\n");

            while (1);
        }
    }

#endif
    int rv = process_funcs[inst->rate_idx](&inst->core, frame, length);
    /*
    if((0 == rv_old)&&(0 < rv))
    {
        printf("++\n");
    }
    if((0 == rv)&&(0 < rv_old))
    {
        printf("--\n");
    }
    rv_old = rv;
    if(rv > 0)
    {
        printf("%d ",rv);
    }*/
    assert(rv >= 0);

    if (rv > 0) {
        rv = 1;
    }

    return rv;
}
