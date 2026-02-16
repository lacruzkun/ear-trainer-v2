#include <arena.h>
#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <defines.h>
#include <midi.c>

#define TSF_IMPLEMENTATION
#include "tsf.h"

bool channel[MIDI_CHANNEL][NUMBER_OF_NOTE];
Sound piano_sound[NUMBER_OF_NOTE];
Sound bass_sound[NUMBER_OF_NOTE];
Sound guitar_sound[NUMBER_OF_NOTE];
ReleaseSound playing_sounds[MAX_PLAYING_SOUND] = { 0 };

void
draw_midi_grid ()
{
  float width = (WINDOW_WIDTH - (2 * PADDING)) / (float)(NUMBER_OF_NOTE + 12);
  float width_spacing
      = (WINDOW_WIDTH - (2 * PADDING)) / (float)(NUMBER_OF_NOTE);
  float height = (WINDOW_HEIGHT - (2 * PADDING)) / (float)(MIDI_CHANNEL + 6);
  float height_spacing
      = (WINDOW_HEIGHT - (2 * PADDING)) / (float)(MIDI_CHANNEL);
  BeginDrawing ();
  ClearBackground (GRAY);
  for (int y = 0; y < MIDI_CHANNEL; y++)
    {
      for (int i = 0; i < NUMBER_OF_NOTE; i++)
        {
          Rectangle rec = { PADDING + i * width_spacing,
                            PADDING + y * height_spacing, width, height };
          DrawRectangleRec (rec, channel[y][i] ? RAYWHITE : BLACK);
        }
    }
  DrawFPS (10, 10);
  EndDrawing ();
}

static tsf *g_sf = NULL;

static void MyAudioCallback(void *bufferData, unsigned int frames)
{
    float *out = (float *)bufferData;

    tsf_render_float(g_sf, out, frames, 0);
}

int
main ()
{
  char midi_file_path[512] = { 0 };
  const char *file_path
      = "../resources/Sonic Generations - Seaside Hills (Modern).mid";
  strcat (midi_file_path, GetApplicationDirectory ());
  strcat (midi_file_path, file_path);
  printf ("%s\n", midi_file_path);
  parse_midi (midi_file_path);

  bool quit = false;
  SetConfigFlags (FLAG_MSAA_4X_HINT);
  SetConfigFlags (FLAG_WINDOW_HIGHDPI);
  InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice ();

  AudioStream stream = LoadAudioStream(SAMPLE_RATE, 32, CHANNELS);

  g_sf = tsf_load_filename("GS Wavetable Synth.sf2");
  if (!g_sf)
  {
      fprintf(stderr, "Failed to load example.sf2\n");
      return 1;
  }

  tsf_set_output(g_sf, TSF_STEREO_INTERLEAVED, SAMPLE_RATE, 0.0f);
  tsf_set_max_voices(g_sf, 128); // pre-allocate voices (good for real-time)

  SetAudioStreamCallback(stream, MyAudioCallback);
  SetAudioStreamVolume(stream, 0.3);

  PlayAudioStream(stream);

  double current_frame = 0;
  double previous_frame = 0;
  double ticks_per_second = (midi.division * 1000000.0) / midi.tempo;

  SetTargetFPS(100);

  while (!WindowShouldClose () && !quit)
    {

      if (IsKeyPressed (KEY_ENTER))
        {
          quit = true;
        }
      previous_frame = current_frame;
      float dt = GetFrameTime ();
      current_frame += dt * ticks_per_second;
      MidiEvent *s;
      u64 start_tick = floor (previous_frame) + 1;
      u64 end_tick = (u64)floor (current_frame);

      for (u64 tick = start_tick; tick <= end_tick; tick++)
        {
          HASH_FIND (hh, midi.events, &tick, sizeof (u64), s);
          if (s == NULL)
            {
              continue;
            }
          for (u32 index = 0; index < s->size; index++)
            {
              EventValue ev = s->event_value[index];
              switch (ev.event_id)
                {
                case NOTE_OFF:
                  {
                    int note = ev.value.note.note;
              u8 program = ev.program;
                    channel[ev.channel][note] = false;
                    if (ev.channel == 9){
                        tsf_bank_note_off(g_sf, 128, 0, note);
                    }else{
                    tsf_note_off(g_sf, program, note);
              }
                  }
                  break;
                case NOTE_ON:
                  {
                    int note = ev.value.note.note;
                    float velocity = (float)ev.value.note.velocity / (float)128;
              u8 program = ev.program;
                    channel[ev.channel][note] = true;
                    if (velocity == 0)
                      {
                        channel[ev.channel][note] = false;
                        tsf_note_off(g_sf, 0, note);
                        break;
                      }
                    if (ev.channel == 9){
                        tsf_bank_note_on(g_sf, 128, 0, note, velocity);
                    }
                    else
                      {
                        tsf_note_on(g_sf, program, note, velocity);
                      }
                  }
                  break;
                default:
                  {
                    continue;
                  }
                }
            }
        }
      draw_midi_grid ();
    }

  return 0;
}
