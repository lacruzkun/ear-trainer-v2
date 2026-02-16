// render_peak_test.c  (run on main thread — not in audio callback)
#define TSF_IMPLEMENTATION
#include "tsf.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    tsf *sf = tsf_load_filename("GS Wavetable Synth.sf2");
    if (!sf) { fprintf(stderr, "sf failed\n"); return 1; }
    tsf_set_output(sf, TSF_STEREO_INTERLEAVED, 44100, 0.0f);
    tsf_set_max_voices(sf, 128);
    tsf_note_on(sf, 0, 60, 1.0f);

    const int frames = 2048;
    float *buf = malloc(sizeof(float) * frames * 2);
    tsf_render_float(sf, buf, frames, 0);

    float peak = 0.0f;
    for (int i = 0; i < frames * 2; ++i) {
        float a = fabsf(buf[i]);
        if (a > peak) peak = a;
    }
    printf("peak = %f\n", peak); // should be <= 1.0 normally. >1 -> clipping
    free(buf);
    tsf_close(sf);
    return 0;
}
