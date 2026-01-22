#include "raylib.h"
#define BACKGROUND_COLOR RAYWHITE

#define BUTTON_COLOR ORANGE
#define BUTTON_OUTLINE_COLOR BLACK
#define BUTTON_HOVER_COLOR PINK
#define BUTTON_SHADOW BLACK

#define ENTRY_SIZE 200
#define ENTRY_COLOR BLACK
#define ENTRY_TEXT_COLOR BLACK
#define MAX_ENTRY_INDEX 128

#define CURSOR_COLOR BLACK
#define FPS 60
#define FONT_SIZE 20

typedef struct {
  bool shadow;
  bool round;
  bool outline;
  bool hover;
  bool is_pressed;
  Rectangle bound;
  Color color;
  Color outline_color;
  Color hover_color;
  Color shadow_color;
} Button;

typedef struct {
  bool selected;
  bool hover;
  Rectangle bound;
  Color color;
  Color text_color;
  char text[128];
  unsigned int current_index;
} Entry;

Entry create_entry() {
  Entry e = {.selected = false,
             .bound = {50, 200, ENTRY_SIZE, 50},
             .color = ENTRY_COLOR,
             .text_color = ENTRY_TEXT_COLOR,
             .current_index = 0,
             .text = {0},
             .hover = false};
  return e;
}

void draw_entry(Entry *e) {
  Entry temp_entry = *e;
  int text_offset = 10;
  int cursor_offset = 15;
  Vector2 cursor_pos = {e->bound.x + cursor_offset, e->bound.y};
  Vector2 text_size = MeasureTextEx(GetFontDefault(), e->text, FONT_SIZE, 1);

  DrawRectangleRounded(e->bound, 1, 4, BACKGROUND_COLOR);
  DrawRectangleRoundedLinesEx(e->bound, 1, 4, 1, e->color);
  if (e->selected) {
    if (IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) {
      if (e->current_index > 0) {
        e->current_index = e->current_index - 1;
        e->text[e->current_index] = 0;
      }
    }

    char c;
    while ((c = GetCharPressed())) {
      if (e->current_index < MAX_ENTRY_INDEX) {
        e->text[e->current_index] = c;
        e->current_index = e->current_index + 1;
      }
    }

    if ((int)(GetTime() * FPS) % FPS < FPS * (3.0 / 4.0)) {
      int cursor_height = e->bound.height / 2;
      int padding = (e->bound.height - cursor_height) / 2;
      DrawLineEx((Vector2){cursor_pos.x + text_size.x, cursor_pos.y + padding},
                 (Vector2){cursor_pos.x + text_size.x,
                           cursor_pos.y + e->bound.height - padding},
                 2, CURSOR_COLOR);
    }
  }

  DrawTextEx(GetFontDefault(), e->text,
             (Vector2){e->bound.x + text_offset,
                       e->bound.y + (e->bound.height / 2) - (text_size.y / 2)},
             FONT_SIZE, 1, ENTRY_TEXT_COLOR);
}

Button create_button() {
  Button b = {.bound = {50, 50, 200, 50},
              .color = BUTTON_COLOR,
              .outline_color = BUTTON_OUTLINE_COLOR,
              .hover_color = BUTTON_HOVER_COLOR,
              .shadow_color = BUTTON_SHADOW,
              .shadow = true,
              .is_pressed = false,
              .round = true,
              .outline = true,
              .hover = false};

  return b;
}

void draw_button(Button b) {
  if (b.hover) {
    b.color = b.hover_color;
  }

  Rectangle shadow = {b.bound.x + 5, b.bound.y + 5, b.bound.width,
                      b.bound.height};

  if (b.is_pressed) {
    b.bound.x = b.bound.x + 10;
    b.bound.y = b.bound.y + 10;
  };

  if (b.round) {
    if (b.shadow) {
      DrawRectangleRounded(shadow, 0.5, 4, b.shadow_color);
    }
    DrawRectangleRounded(b.bound, 0.5, 4, b.color);
  } else {
    if (b.shadow) {
      DrawRectangleRec(shadow, b.shadow_color);
    }
    DrawRectangleRec(b.bound, b.color);
  }

  if (b.outline) {
    DrawRectangleRoundedLinesEx(b.bound, 0.5, 4, 2, BLACK);
  }
}
