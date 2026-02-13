#include <arena.h>
#include <assert.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uthash.h>
#include <wchar.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BIN_DIRECTORY GetApplicationDirectory ()

#define MIDI_CHANNEL 16
#define NUMBER_OF_NOTE 128
#define PADDING 50

/* The structure of the midi data structure there will be a hash map the key of
 * the hash map corresponds to the delta time of an event the value of the hash
 * map is a dynamic array or linked list of all the events to be executed at
 * that particular deltatime the structure will have a field for the notes of
 * an instrument or should i say channel.
 *
 * 16 channels each has 128 notes from 0 representing C at -1 octave
 */

//

typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned char u8;
bool channel[MIDI_CHANNEL][NUMBER_OF_NOTE];
Sound piano_sound[NUMBER_OF_NOTE];
Sound bass_sound[NUMBER_OF_NOTE];
bool end_of_track = false;
u64 current_frame = 0;
u64 previous_frame = 0;
void LoadPiano (void);
void LoadBass (void);

typedef enum
{
  TEXT_EVENT = 0x01,
  COPYRIGHT = 0x02,
  TRACK_NAME = 0x03,
  INST_NAME = 0x04,
  LYRIC = 0x05,
  MARKER = 0x06,
  CUE_POINT = 0x07,
  SEQUENCE_NUMBER = 0x00,
  CHANNEL_PREFIX = 0x20,
  END_OF_TRACK = 0x2F,
  SET_TEMPO = 0x51,
  SMPTE_OFFSET = 0x54,
  TIME_SIGNATURE = 0x58,
  KEY_SIGNATURE = 0x59,
  DEVICE_PORT_NAME = 0x09,
  MIDI_PORT = 0x21,
} MetaEventType;

typedef enum
{
  NOTE_OFF = 0x8,
  NOTE_ON = 0x9,
  POLY_KEY_PRESSURE = 0xA,
  CTRL_CHANGE = 0xB,
  PROG_CHANGE = 0xc,
  CHANNEL_PRESSURE = 0xD,
  PITCH_WHEEL = 0xE
} BasicEventType;

typedef enum
{
  SYS_EXCLUSIVE = 0xF0,
  TIME_CODE_QUARTER_FRAME = 0xF1,
  SONG_POSITION_POINTER = 0xF2,
  SONG_SELECT = 0xF3,
  TUNE_REQUEST = 0xF6,
  SYS_EXCLUSIVE_END = 0xF7,
} CommonSysEventType;

typedef enum
{
  SYS_EVENT,
  BASIC_EVENT,
  META_EVENT,
} MidiEventType;

typedef struct
{
  MidiEventType event_type;
  u8 event_id;
  u8 program;
  u8 channel;
  union
  {
    char *text;
    u8 number;
    u8 position;
    u8 data;
    u8 channel;
    u32 tempo;
    struct smpte_offset
    {
      u8 hout;
      u8 minute;
      u8 sec;
      u8 frame;
      u8 frac;
    } smpte_offset;

    struct time_signature
    {
      u8 num;
      u8 den;
      u8 clock;
      u8 byte;
    } time_signature;

    struct key_signature
    {
      u8 sf;
      u8 mi;
    } key_signature;

    u64 sys_exclusiv;

    struct time_code
    {
      u8 data;
      u8 type;
      u8 value;
    } time_code;

    struct song_pos
    {
      u8 LSB;
      u8 MSB;
    } song_pos;

    struct pitch_wheel
    {
      u8 LSB;
      u8 MSB;
    } pitch_wheel;

    struct note
    {
      u8 note;
      u8 velocity;
    } note;

    struct poly_key
    {
      u8 note;
      u8 pressure;
    } poly_key;

    struct ctrl_change
    {
      u8 control;
      u8 value;
    } ctrl_change;

    u8 program;
    u8 pressure;
  } value;
} EventValue;

// node
// since event_value is a pointer make sure to always initialize it with only
// one item if not it might cause seg fault
typedef struct
{
  // key
  u64 delta_time;
  // value
  EventValue *event_value;
  u32 size;
  UT_hash_handle hh;
} MidiEvent;

typedef struct
{
  MidiEvent *events;
  u32 size;
  u32 capacity;
  u32 delta_time;
  u64 c_tick;
  u32 Division;
  u8 number_of_tracks;
  int TEMPO;
} Midi;

Midi midi = { 0 };

void
add_event_to_hashmap (Midi *m, u64 d, EventValue e)
{
  m->delta_time = m->delta_time + d;
  MidiEvent *s;
  HASH_FIND (hh, m->events, &m->delta_time, sizeof (u64), s);
  if (s == NULL)
    {
      s = (MidiEvent *)malloc (sizeof (MidiEvent));
      s->delta_time = m->delta_time;
      s->size = 1;
      s->event_value = (EventValue *)malloc (sizeof (EventValue) * s->size);
      s->event_value[s->size - 1] = e;
      HASH_ADD (hh, m->events, delta_time, sizeof (u64), s);
    }
  else
    {
      s->size += 1;
      s->event_value = (EventValue *)realloc (s->event_value,
                                              sizeof (EventValue) * s->size);
      s->event_value[s->size - 1] = e;
    }
}
char *
file_read_string (int size, FILE *file)
{
  char *buffer = malloc ((size + 1) * sizeof (char));
  fread (buffer, sizeof (char), size, file);
  buffer[size] = '\0';
  return buffer;
}
u64
file_read_byte (int size, FILE *file)
{
  assert (size <= 8 && size > 0);
  u8 *buffer = malloc (size * sizeof (u8));
  u64 result = 0;
  fread (buffer, sizeof (u8), size, file);
  for (int i = 0; i < size; i++)
    {
      result = result << 0x08;
      result = result | buffer[i];
    }
  return result;
}

u32
file_read_u32 (FILE *file)
{
  u8 buffer[4] = { 0 };
  u32 len = sizeof (u32);
  u32 result = 0;
  fread (&buffer, sizeof (unsigned char), len, file);
  for (u32 i = 0; i < len; i++)
    {
      result = result << 0x08;
      result = result | buffer[i];
    }
  return result;
}

u16
file_read_u16 (FILE *file)
{
  u8 buffer[4] = { 0 };
  unsigned int len = sizeof (u16);
  u16 result = 0;
  assert (fread (&buffer, sizeof (u8), len, file) == len);
  for (unsigned int i = 0; i < len; i++)
    {
      result = result << 0x08;
      result = result | buffer[i];
    }
  return result;
}

u8
file_read_u8 (FILE *file)
{
  u8 buffer = 0;
  unsigned int len = sizeof (u8);
  u8 result = 0;
  assert (fread (&buffer, 1, 1, file) == len);
  result = buffer;
  return result;
}

u8
file_peek_u8 (FILE *file)
{
  u8 buffer;
  u8 result = 0;
  assert (fread (&buffer, 1, 1, file) == 1);
  result = buffer;
  fseek (file, -1, SEEK_CUR);
  return result;
}

u64
file_read_variable_length (FILE *file)
{
  u8 buffer;
  unsigned int len = sizeof (u8);
  u64 temp = 0;
  u32 count = 0;
  for (unsigned int i = 0; i < sizeof (u64); i++)
    {
      fread (&buffer, sizeof (unsigned char), len, file);
      temp = temp << 8;
      temp = temp | buffer;
      count++;
      if (!(buffer & 0x80))
        {
          break;
        }
    }
  assert (count <= 8);

  buffer = 0;
  u64 result = 0;
  for (unsigned int i = 0; i < 8; i++)
    {
      buffer = temp >> (56 - i * 8);
      buffer = buffer & 0x7F;
      result = result << 7;
      result = result | buffer;
    }
  return result;
}

/* Structure of midi file, first 4 bytes: identifier, next 2 bytes the size of
 * the header, next 2 bytes the midi format, next 2 bytes number of tracks,
 * next 2 bytes, number of divisions, then a list of events*/

void
file_read_event (FILE *file, u64 delta_time, u8 status, u8 *program_m)
{
  printf ("%-20s %llx\n", "EVENT TIME", delta_time);
  if ((status & 0xF0) == 0xF0)
    {
      if (status == 0xFF)
        {

          // meta event
          u8 type = file_read_u8 (file);
          switch (type)
            {
            case TEXT_EVENT:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT",
                        "TEXT EVENT", data);
              }
              break;
            case COPYRIGHT:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT", "COPYRIGHT",
                        data);
              }
              break;
            case TRACK_NAME:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT",
                        "TRACK NAME", data);
              }
              break;
            case INST_NAME:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT",
                        "INSTURMENT NAME", data);
              }
              break;
            case LYRIC:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT", "LYRICS",
                        data);
              }
              break;
            case MARKER:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT", "MARKER",
                        data);
              }
              break;
            case CUE_POINT:
              {
                u64 length = file_read_variable_length (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s text: %-20s\n", "META EVENT", "CUE POINT",
                        data);
              }
              break;
            case SEQUENCE_NUMBER:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x01);
                char *number = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.number = 1 };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s number: %s\n", "META EVENT",
                        "SEQUENCE NUMBER", number);
              }
              break;
            case CHANNEL_PREFIX:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x01);
                u8 channel = file_read_u8 (file);
                EventValue event_value = { .program = *program_m,
                                           .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.channel = channel };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s channel: %02x\n", "META EVENT",
                        "CHANNEL PREFIX", channel);
              }
              break;
            case MIDI_PORT:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x01);
                u8 port = file_read_u8 (file);
                EventValue event_value = { .program = *program_m,
                                           .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.channel = port };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s channel: %02x\n", "META EVENT",
                        "MIDI PORT", port);
              }
              break;
            case END_OF_TRACK:
              {
                u8 length = file_read_u8 (file);
                printf ("%x \n", ftell (file));
                assert (length == 0x00);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value = { 0 } };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s\n", "META EVENT", "END OF TRACK");
                midi.delta_time = 0;
                end_of_track = true;
              }
              break;
            case SET_TEMPO:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x03);
                u32 tempo = file_read_byte (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.tempo = tempo };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s tempo: %02x\n", "META EVENT", "SET TEMPO",
                        tempo);
              }
              break;
            case SMPTE_OFFSET:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x05);
                u8 hour = file_read_u8 (file);
                u8 minute = file_read_u8 (file);
                u8 sec = file_read_u8 (file);
                u8 frame = file_read_u8 (file);
                u8 frac = file_read_u8 (file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.smpte_offset = {
                                               hour,
                                               minute,
                                               sec,
                                               frame,
                                               frac,
                                           } };

                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s hour: %02x, minute: "
                        "%02x, sec: %02x, frame: %02x, "
                        "frac: %02x\n",
                        "META EVENT", "SMPTE_OFFSET", hour, minute, sec, frame,
                        frac);
              }
              break;
            case TIME_SIGNATURE:
              {
                u8 length = file_read_u8 (file);
                printf ("%x \n", ftell (file));
                assert (length == 0x04);
                u8 num = file_read_u8 (file);
                u8 den = file_read_u8 (file);
                u8 clock = file_read_u8 (file);
                u8 byte = file_read_u8 (file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.time_signature = {
                                               num,
                                               den,
                                               clock,
                                               byte,
                                           } };

                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s num: %02x, den: %02x, "
                        "clock: %02x, byte: %02x\n",
                        "META EVENT", "TIME_SIGNATURE", num, den, clock, byte);
              }
              break;
            case KEY_SIGNATURE:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x02);
                u8 sf = file_read_u8 (file);
                u8 mi = file_read_u8 (file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.key_signature = { sf, mi } };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s sf: %02x, mi: %02x\n", "META EVENT",
                        "KEY_SIGNATURE", sf, mi);
              }
              break;
            case DEVICE_PORT_NAME:
              {
                u8 length = file_read_u8 (file);
                char *data = file_read_string (length, file);
                EventValue event_value = { .event_type = META_EVENT,
                                           .event_id = type,
                                           .value.text = data };
                add_event_to_hashmap (&midi, delta_time, event_value);
                printf ("%-20s %-20s Device port name: %-20s\n", "META EVENT",
                        "DEVICE_PORT_NAME", data);
              }
              break;
            }
        }
      else
        {
          // common system event

          switch (status)
            {

            case SYS_EXCLUSIVE:
              {
                u8 length = file_read_u8 (file);
                u64 data = file_read_byte (length, file);
                printf ("%-20s %-20s  data: %02llx\n", "COMMON SYSTEM EVENT",
                        "SYS_EXCLUSIVE", data);
              }
              break;
            case TIME_CODE_QUARTER_FRAME:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x01);
                u8 data = file_read_u8 (file);
                u8 type = data >> 4;
                u8 value = data & 0x0F;
                printf ("%-20s %-20s  type: %02x value: %02x\n",
                        "COMMON SYSTEM EVENT", "TIME_CODE_QUARTER_FRAME", type,
                        value);
              }
              break;
            case SONG_POSITION_POINTER:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x02);
                u8 LSB = file_read_u8 (file);
                u8 MSB = file_read_u8 (file);
                printf ("%-20s %-20s  LSB: %02x MSB: %02x\n",
                        "COMMON SYSTEM EVENT", "SONG_POSITION_POINTER", LSB,
                        MSB);
              }
              break;
            case SONG_SELECT:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x01);
                u8 data = file_read_u8 (file);
                printf ("%-20s %-20s  data: %02x\n", "COMMON SYSTEM EVENT",
                        "SONG_SELECT", data);
              }
              break;
            case TUNE_REQUEST:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x00);
              }
              break;
            case SYS_EXCLUSIVE_END:
              {
                u8 length = file_read_u8 (file);
                assert (length == 0x00);
              }
              break;
            }
        }
    }
  else
    {
      // basic events
      u8 channel = status & 0x0F;
      printf ("%-20s %02x\n", "CHANNEL", channel);
      u8 basic_event = status >> 4;
      switch (basic_event)
        {

        case NOTE_OFF:
          {
            u8 note = file_read_u8 (file);
            u8 velocity = file_read_u8 (file);
            EventValue event_value = { .program = *program_m,
                                       .channel = channel,
                                       .event_type = BASIC_EVENT,
                                       .event_id = basic_event,
                                       .value.note = { note, velocity } };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  note: %02x velocity: %02x\n",
                    "BASIC MIDI EVENT", "NOTE OFF", note, velocity);
          }
          break;
        case NOTE_ON:
          {
            u8 note = file_read_u8 (file);
            u8 velocity = file_read_u8 (file);
            EventValue event_value = { .program = *program_m,
                                       .channel = channel,
                                       .event_type = BASIC_EVENT,
                                       .event_id = basic_event,
                                       .value.note = { note, velocity } };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  note: %02x velocity: %02x\n",
                    "BASIC MIDI EVENT", "NOTE ON", note, velocity);
          }
          break;
        case POLY_KEY_PRESSURE:
          {
            u8 note = file_read_u8 (file);
            u8 pressure = file_read_u8 (file);
            EventValue event_value = { .program = *program_m,
                                       .event_type = BASIC_EVENT,
                                       .event_id = basic_event,
                                       .value.poly_key = { note, pressure } };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  note: %02x pressure: %02x\n",
                    "BASIC MIDI EVENT", "POLY_KEY_PRESSURE", note, pressure);
          }
          break;
        case CTRL_CHANGE:
          {
            u8 control = file_read_u8 (file);
            u8 value = file_read_u8 (file);
            EventValue event_value
                = { .program = *program_m,
                    .event_type = BASIC_EVENT,
                    .event_id = basic_event,
                    .value.ctrl_change = { control, value } };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  control: %02x value: %02x\n",
                    "BASIC MIDI EVENT", "CTRL_CHANGE", control, value);
          }
          break;
        case PROG_CHANGE:
          {
            u8 program = file_read_u8 (file);
            *program_m = program;
            EventValue event_value = { .event_type = BASIC_EVENT,
                                       .program = *program_m,
                                       .event_id = basic_event,
                                       .value.program = program };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  program: %02x\n", "BASIC MIDI EVENT",
                    "PROG_CHANGE", program);
          }
          break;
        case CHANNEL_PRESSURE:
          {
            u8 pressure = file_read_u8 (file);
            EventValue event_value = { .program = *program_m,
                                       .event_type = BASIC_EVENT,
                                       .event_id = basic_event,
                                       .value.pressure = pressure };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  pressure: %02x\n", "BASIC MIDI EVENT",
                    "CHANNEL_PRESSURE", pressure);
          }
          break;
        case PITCH_WHEEL:
          {
            u8 LSB = file_read_u8 (file);
            u8 MSB = file_read_u8 (file);
            EventValue event_value = { .program = *program_m,
                                       .event_type = BASIC_EVENT,
                                       .event_id = basic_event,
                                       .value.pitch_wheel = { LSB, MSB } };
            add_event_to_hashmap (&midi, delta_time, event_value);
            printf ("%-20s %-20s  LSB: %02x MSB: %02x\n", "BASIC MIDI EVENT",
                    "PITCH_WHEEL", LSB, MSB);
          }
          break;
        default:
          file_read_u8 (file);
        }
    }
}

void
parse_midi (const char *file_path)
{
  FILE *midi_file;
  midi_file = fopen (file_path, "rb");
  if (midi_file == NULL)
    {
      printf ("Cannot open file %s\n", file_path);
    }

  fseek (midi_file, 0, SEEK_END);
  long file_len = ftell (midi_file);
  rewind (midi_file);
  char *header_identifier = file_read_string (4, midi_file);
  u32 header_size = file_read_u32 (midi_file);
  u16 midi_format = file_read_u16 (midi_file);
  u16 number_of_tracks = file_read_u16 (midi_file);

  // division -> how many ticks per quater note
  u16 division = file_read_u16 (midi_file);

  printf ("HEADER %-20s\n", header_identifier);
  printf ("HEADER Size %04x\n", header_size);
  printf ("MIDI FORMAT  %04x\n", midi_format);
  printf ("NUMBER OF TRACKS  %04x\n", number_of_tracks);
  printf ("DIVISION  %04x\n", division);

  while ((ftell (midi_file) < file_len))
    {
      char *identifier = file_read_string (4, midi_file);
      u32 size = file_read_u32 (midi_file);
      long chunk_end = ftell (midi_file) + size;
      u8 program_m = 0;

      if (strcmp (identifier, "MTrk") == 0)
        {
          printf ("MTrk\n\n\n");
          u8 running_status = 0;
          while ((ftell (midi_file) < chunk_end))
            {
              u64 delta_time = file_read_variable_length (midi_file);
              // printf("Current cursor byte %i\n",
              // ftell(midi_file)); printf("chunk end  byte
              // %i\n", chunk_end);

              u8 next_byte = file_peek_u8 (midi_file);

              u8 status = running_status;
              // any byte & 0x80 results in 0 if the byte is <
              // 0x80 and results in 0x80 if the byte is greater
              // than 0x80. we use 0x80 because it's a byte that
              // has 1 as the MSB and the rest bytes is 0
              if (next_byte & 0x80)
                {
                  status = file_read_u8 (midi_file);
                  running_status = status;
                }
              file_read_event (midi_file, delta_time, status, &program_m);
            }
        }
      else
        {
          fseek (midi_file, chunk_end, SEEK_CUR);
        }
    }

  free (header_identifier);
}

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

int
main ()
{
  //
  //  Midi Stuff
  //
  //
  char midi_file_path[512] = { 0 };
  // const char *file_path = "../resources/Super Mario 64 - Medley.mid";
  const char *file_path = "../resources/Sonic Colors - Reach for the Stars.mid";
  strcat (midi_file_path, GetApplicationDirectory ());
  strcat (midi_file_path, file_path);
  printf ("%s\n", midi_file_path);
  parse_midi (midi_file_path);
  MidiEvent *m = NULL;
  MidiEvent *h = malloc (sizeof (MidiEvent));
  h->delta_time = 10;
  EventValue e = {
    .event_type = BASIC_EVENT,
    .event_id = 0xF3,
    .value.text = "Hello",
  };
  h->event_value = &e;
  h->size = 1;
  HASH_ADD (hh, m, delta_time, sizeof (u64), h);

  MidiEvent *s;
  for (s = midi.events; s != NULL; s = s->hh.next)
    {
      printf ("events from hashmap \n");
      printf ("delta time %llu\n", s->delta_time);
      for (int i = 0; i < s->size; i++)
        {
          printf ("event [%i] id %02x\n", i, s->event_value[i].event_id);
        }
      printf ("size of dynamic array %u\n", s->size);
    }
  int num_of_events = HASH_COUNT (midi.events);
  printf ("number of event is %i\n", num_of_events);

  bool quit = false;

  //
  //  Drawing Stuff
  //
  //
  SetConfigFlags (FLAG_MSAA_4X_HINT);
  SetConfigFlags (FLAG_WINDOW_HIGHDPI);
  InitWindow (WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice ();
  LoadPiano ();
  // LoadBass ();

  SetTargetFPS(60);

  while (!WindowShouldClose () && !quit)
    {

      if (IsKeyPressed (KEY_ENTER))
        {
          quit = true;
        }
      previous_frame = current_frame;
      current_frame += 5;
      MidiEvent *s;
      for (u64 i = previous_frame; i < current_frame; i++)
        {
          HASH_FIND (hh, midi.events, &i, sizeof (u64), s);
          if (s != NULL)
            {
              for (int i = 0; i < s->size; i++)
                {
                  switch (s->event_value[i].event_id)
                    {
                    case NOTE_OFF:
                      {
                        channel[s->event_value[i].channel]
                               [s->event_value[i].value.note.note]
                            = false;
                        StopSound (
                            piano_sound[s->event_value[i].value.note.note]);
                      }
                      break;
                    case NOTE_ON:
                      {
                        channel[s->event_value[i].channel]
                               [s->event_value[i].value.note.note]
                            = true;
                        if (s->event_value[i].value.note.velocity == 0)
                          {
                            channel[s->event_value[i].channel]
                                   [s->event_value[i].value.note.note]
                                = false;
                            StopSound (piano_sound[s->event_value[i]
                                                       .value.note.note]);
                          }
                        else
                          {
                            SetSoundVolume (
                                piano_sound[s->event_value[i].value.note.note],
                                (float)s->event_value[i].value.note.velocity
                                        / (float)0xFF
                                    + 0x0F);
                            PlaySound (piano_sound[s->event_value[i]
                                                       .value.note.note]);
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
        }
      draw_midi_grid ();
    }

  return 0;
}

void
LoadPiano (void)
{
  piano_sound[21] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/0-a.mp3"));
  piano_sound[22] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/0-as.mp3"));
  piano_sound[23] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/0-b.mp3"));
  piano_sound[24] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-c.mp3"));
  piano_sound[25] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-cs.mp3"));
  piano_sound[26] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-d.mp3"));
  piano_sound[27] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-ds.mp3"));
  piano_sound[28] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-e.mp3"));
  piano_sound[29] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-f.mp3"));
  piano_sound[30] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-fs.mp3"));
  piano_sound[31] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-g.mp3"));
  piano_sound[32] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-gs.mp3"));
  piano_sound[33] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-a.mp3"));
  piano_sound[34] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-as.mp3"));
  piano_sound[35] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/1-b.mp3"));
  piano_sound[36] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-c.mp3"));
  piano_sound[37] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-cs.mp3"));
  piano_sound[38] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-d.mp3"));
  piano_sound[39] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-ds.mp3"));
  piano_sound[40] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-e.mp3"));
  piano_sound[41] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-f.mp3"));
  piano_sound[42] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-fs.mp3"));
  piano_sound[43] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-g.mp3"));
  piano_sound[44] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-gs.mp3"));
  piano_sound[45] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-a.mp3"));
  piano_sound[46] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-as.mp3"));
  piano_sound[47] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/2-b.mp3"));
  piano_sound[48] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-c.mp3"));
  piano_sound[49] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-cs.mp3"));
  piano_sound[50] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-d.mp3"));
  piano_sound[51] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-ds.mp3"));
  piano_sound[52] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-e.mp3"));
  piano_sound[53] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-f.mp3"));
  piano_sound[54] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-fs.mp3"));
  piano_sound[55] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-g.mp3"));
  piano_sound[56] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-gs.mp3"));
  piano_sound[57] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-a.mp3"));
  piano_sound[58] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-as.mp3"));
  piano_sound[59] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/3-b.mp3"));
  piano_sound[60] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-c.mp3"));
  piano_sound[61] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-cs.mp3"));
  piano_sound[62] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-d.mp3"));
  piano_sound[63] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-ds.mp3"));
  piano_sound[64] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-e.mp3"));
  piano_sound[65] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-f.mp3"));
  piano_sound[66] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-fs.mp3"));
  piano_sound[67] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-g.mp3"));
  piano_sound[68] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-gs.mp3"));
  piano_sound[69] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-a.mp3"));
  piano_sound[70] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-as.mp3"));
  piano_sound[71] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/4-b.mp3"));
  piano_sound[72] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-c.mp3"));
  piano_sound[73] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-cs.mp3"));
  piano_sound[74] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-d.mp3"));
  piano_sound[75] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-ds.mp3"));
  piano_sound[76] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-e.mp3"));
  piano_sound[77] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-f.mp3"));
  piano_sound[78] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-fs.mp3"));
  piano_sound[79] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-g.mp3"));
  piano_sound[80] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-gs.mp3"));
  piano_sound[81] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-a.mp3"));
  piano_sound[82] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-as.mp3"));
  piano_sound[83] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/5-b.mp3"));
  piano_sound[84] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-c.mp3"));
  piano_sound[85] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-cs.mp3"));
  piano_sound[86] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-d.mp3"));
  piano_sound[87] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-ds.mp3"));
  piano_sound[88] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-e.mp3"));
  piano_sound[89] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-f.mp3"));
  piano_sound[90] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-fs.mp3"));
  piano_sound[91] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-g.mp3"));
  piano_sound[92] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-gs.mp3"));
  piano_sound[93] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-a.mp3"));
  piano_sound[94] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-as.mp3"));
  piano_sound[95] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/6-b.mp3"));
  piano_sound[96] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-c.mp3"));
  piano_sound[97] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-cs.mp3"));
  piano_sound[98] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-d.mp3"));
  piano_sound[99] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-ds.mp3"));
  piano_sound[100] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-e.mp3"));
  piano_sound[101] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-f.mp3"));
  piano_sound[102] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-fs.mp3"));
  piano_sound[103] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-g.mp3"));
  piano_sound[104] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-gs.mp3"));
  piano_sound[105] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-a.mp3"));
  piano_sound[106] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-as.mp3"));
  piano_sound[107] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/piano/7-b.mp3"));
}

void
LoadBass (void)
{
  bass_sound[21] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-a.mp3"));
  SetSoundPitch (bass_sound[21], -2);
  bass_sound[22] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-as.mp3"));
  SetSoundPitch (bass_sound[22], -2);
  bass_sound[23] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-b.mp3"));
  SetSoundPitch (bass_sound[23], -2);
  bass_sound[24] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-c.mp3"));
  SetSoundPitch (bass_sound[24], -1);
  bass_sound[25] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-cs.mp3"));
  SetSoundPitch (bass_sound[25], -1);
  bass_sound[26] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-d.mp3"));
  SetSoundPitch (bass_sound[26], -1);
  bass_sound[27] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-ds.mp3"));
  SetSoundPitch (bass_sound[27], -1);
  bass_sound[28] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-e.mp3"));
  SetSoundPitch (bass_sound[28], -1);
  bass_sound[29] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-f.mp3"));
  SetSoundPitch (bass_sound[29], -1);
  bass_sound[30] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-fs.mp3"));
  SetSoundPitch (bass_sound[30], -1);
  bass_sound[31] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-g.mp3"));
  SetSoundPitch (bass_sound[31], -1);
  bass_sound[32] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-gs.mp3"));
  SetSoundPitch (bass_sound[32], -1);
  bass_sound[33] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-a.mp3"));
  SetSoundPitch (bass_sound[33], -1);
  bass_sound[34] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-as.mp3"));
  SetSoundPitch (bass_sound[34], -1);
  bass_sound[35] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-b.mp3"));
  SetSoundPitch (bass_sound[35], -1);
  bass_sound[36] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-c.mp3"));
  bass_sound[37] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-cs.mp3"));
  bass_sound[38] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-d.mp3"));
  bass_sound[39] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-ds.mp3"));
  bass_sound[40] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-e.mp3"));
  bass_sound[41] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-f.mp3"));
  bass_sound[42] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-fs.mp3"));
  bass_sound[43] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-g.mp3"));
  bass_sound[44] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-gs.mp3"));
  bass_sound[45] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-a.mp3"));
  bass_sound[46] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-as.mp3"));
  bass_sound[47] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-b.mp3"));
  bass_sound[48] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-c.mp3"));
  SetSoundPitch (bass_sound[48], 2);
  bass_sound[49] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-cs.mp3"));
  SetSoundPitch (bass_sound[49], 2);
  bass_sound[50] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-d.mp3"));
  SetSoundPitch (bass_sound[50], 2);
  bass_sound[51] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-ds.mp3"));
  SetSoundPitch (bass_sound[51], 2);
  bass_sound[52] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-e.mp3"));
  SetSoundPitch (bass_sound[52], 2);
  bass_sound[53] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-f.mp3"));
  SetSoundPitch (bass_sound[53], 2);
  bass_sound[54] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-fs.mp3"));
  SetSoundPitch (bass_sound[54], 2);
  bass_sound[55] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-g.mp3"));
  SetSoundPitch (bass_sound[55], 2);
  bass_sound[56] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-gs.mp3"));
  SetSoundPitch (bass_sound[56], 2);
  bass_sound[57] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-a.mp3"));
  SetSoundPitch (bass_sound[57], 2);
  bass_sound[58] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-as.mp3"));
  SetSoundPitch (bass_sound[58], 2);
  bass_sound[59] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-b.mp3"));
  SetSoundPitch (bass_sound[59], 2);
  bass_sound[60] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-c.mp3"));
  SetSoundPitch (bass_sound[60], 3);
  bass_sound[61] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-cs.mp3"));
  SetSoundPitch (bass_sound[61], 3);
  bass_sound[62] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-d.mp3"));
  SetSoundPitch (bass_sound[62], 3);
  bass_sound[63] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-ds.mp3"));
  SetSoundPitch (bass_sound[63], 3);
  bass_sound[64] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-e.mp3"));
  SetSoundPitch (bass_sound[64], 3);
  bass_sound[65] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-f.mp3"));
  SetSoundPitch (bass_sound[65], 3);
  bass_sound[66] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-fs.mp3"));
  SetSoundPitch (bass_sound[66], 3);
  bass_sound[67] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-g.mp3"));
  SetSoundPitch (bass_sound[67], 3);
  bass_sound[68] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-gs.mp3"));
  SetSoundPitch (bass_sound[68], 3);
  bass_sound[69] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-a.mp3"));
  SetSoundPitch (bass_sound[69], 3);
  bass_sound[70] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-as.mp3"));
  SetSoundPitch (bass_sound[70], 3);
  bass_sound[71] = LoadSound (TextFormat (
      "%s/%s", BIN_DIRECTORY, "../resources/instrument/bass/2-b.mp3"));
  SetSoundPitch (bass_sound[71], 3);
}
