#include <arena.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BIN_DIRECTORY GetApplicationDirectory()

#define MIDI_CHANNEL 16
#define NUMBER_OF_NOTE 128
#define PADDING 50

/* The structure of the midi data structure there will be a hash map the key of
 * the hash map corresponds to the delta time of an event the value of the hash
 * map is a dynamic array or linked list of all the events to be executed at
 * that particular deltatime the structure will have a field for the notes of an
 * instrument or should i say channel.
 *
 * 16 channels each has 128 notes from 0 representing C at -1 octave
 */

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
bool channel[MIDI_CHANNEL][NUMBER_OF_NOTE];

char *file_read_string(int size, FILE *file) {
  char *buffer = malloc(size * sizeof(char));
  fread(buffer, sizeof(char), size, file);
  return buffer;
}

u32 file_read_u32(FILE *file) {
  u8 buffer[4];
  unsigned int len = sizeof(u32);
  u32 result = 0;
  fread(&buffer, sizeof(unsigned char), len, file);
  for (int i = 0; i < len; i++) {
    result = result << 0x08;
    result = result | buffer[i];
  }
  return result;
}

u16 file_read_u16(FILE *file) {
  u8 buffer[4];
  unsigned int len = sizeof(u16);
  u16 result = 0;
  fread(&buffer, sizeof(unsigned char), len, file);
  for (unsigned int i = 0; i < len; i++) {
    result = result << 0x08;
    result = result | buffer[i];
  }
  return result;
}

/* Structure of midi file, first 4 bytes: identifier, next 2 bytes the size of
 * the header, next 2 bytes the midi format, next 2 bytes number of tracks, next
 * 2 bytes, number of divisions, then a list of events*/

void parse_midi(const char *file_path) {
  FILE *midi_file;
  midi_file = fopen(file_path, "r");
  if (midi_file == NULL) {
    printf("Cannot open file %s\n", file_path);
  }

  char *header_identifier = file_read_string(4, midi_file);
  u32 header_size = file_read_u32(midi_file);
  u16 midi_format = file_read_u16(midi_file);
  u16 number_of_tracks = file_read_u16(midi_file);
  u16 division = file_read_u16(midi_file);

  printf("%s\n", header_identifier);
  printf("header identifier: %08x\n", header_size);
  printf("midi format: %08x\n", midi_format);
  printf("number of tracks : %08x\n", number_of_tracks);
  printf("division : %08x\n", division);
  free(header_identifier);
}

void draw_midi_grid() {
  float width = (WINDOW_WIDTH - (2 * PADDING)) / (float)(NUMBER_OF_NOTE + 12);
  float width_spacing =
      (WINDOW_WIDTH - (2 * PADDING)) / (float)(NUMBER_OF_NOTE);
  float height = (WINDOW_HEIGHT - (2 * PADDING)) / (float)(MIDI_CHANNEL + 6);
  float height_spacing =
      (WINDOW_HEIGHT - (2 * PADDING)) / (float)(MIDI_CHANNEL);
  BeginDrawing();
  ClearBackground(GRAY);
  for (int y = 0; y < MIDI_CHANNEL; y++) {
    for (int i = 0; i < NUMBER_OF_NOTE; i++) {
      Rectangle rec = {PADDING + i * width_spacing,
                       PADDING + y * height_spacing, width, height};
      DrawRectangleRec(rec, channel[y][i] ? RAYWHITE : BLACK);
    }
  }
  DrawFPS(10, 10);
  EndDrawing();
}

int main() {
  char midi_file_path[512] = {0};
  const char *file_path = "../resources/Super Mario 64 - Medley.mid";
  strcat(midi_file_path, GetApplicationDirectory());
  strcat(midi_file_path, file_path);
  printf("%s\n", midi_file_path);
  parse_midi(midi_file_path);

  channel[4][48] = true;
  channel[4][64] = true;
  channel[4][55] = true;

  bool quit = false;

  return 0;
}
