// audio_callback_example.c
#include "raylib.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

#include <stdio.h>

#define SAMPLE_RATE 44100
#define CHANNELS 2

// Global TinySoundFont pointer (callback has no userdata param, so keep it global)
static tsf *g_sf = NULL;

// Audio thread callback: called on audio thread — must be real-time safe
static void MyAudioCallback(void *bufferData, unsigned int frames)
{
    // raylib/miniaudio default device format is float32 (ma_format_f32),
    // and AUDIO_DEVICE_CHANNELS is usually 2 (stereo).
    // So bufferData is a float* with frames * channels samples.
    float *out = (float *)bufferData;

    // Render 'frames' frames (tsf_render_float writes frames * channels floats).
    // The last arg is 'sinc' interpolation (0 = linear or default — check tsf.h).
    tsf_render_float(g_sf, out, frames, 0);
}
int main(void)
{
    // --- window/audio init ---
    InitWindow(640, 200, "SetAudioStreamCallback example");
    InitAudioDevice();

    // Create a streaming audio stream that matches device format:
    // sampleRate, sampleSize(in bits) = 32 for float, channels
    AudioStream stream = LoadAudioStream(SAMPLE_RATE, 32, CHANNELS);

    // Load SF2 and configure TinySoundFont
    g_sf = tsf_load_filename("GS Wavetable Synth.sf2");
    if (!g_sf)
    {
        fprintf(stderr, "Failed to load example.sf2\n");
        return 1;
    }
    // Make TSF output float interleaved stereo at SAMPLE_RATE
    tsf_set_output(g_sf, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
    tsf_set_max_voices(g_sf, 128); // pre-allocate voices (good for real-time)

    // Attach the audio callback for this stream
    SetAudioStreamCallback(stream, MyAudioCallback);

    // Start playing the stream (audio thread will call MyAudioCallback)
    PlayAudioStream(stream);

    // Example: trigger a note from the main thread (safe if you don't call often)
    tsf_note_on(g_sf, 0, 60, 1.0f); // preset 0, midi note 60 (middle C), velocity 1.0
    tsf_note_on(g_sf, 0, 64, 1.0f); // preset 0, midi note 60 (middle C), velocity 1.0

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Playing SF2 via TinySoundFont -> raylib callback", 20, 80, 10, GRAY);
        EndDrawing();
    }

    // cleanup
    StopAudioStream(stream);
    UnloadAudioStream(stream);
    tsf_close(g_sf);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
