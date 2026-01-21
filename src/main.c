#include <raylib.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define BUTTON_COLOR ORANGE
#define BUTTON_OUTLINE_COLOR BLACK
#define BUTTON_HOVER_COLOR PINK
#define BUTTON_SHADOW BLACK

#define ENTRY_SIZE 200
#define ENTRY_COLOR BLACK
#define ENTRY_TEXT_COLOR BLACK
#define MAX_ENTRY_INDEX 128

#define CURSOR_COLOR BLACK

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
  Vector2 cursor_pos = {e->bound.x + 5, e->bound.y};
  if (e->selected) {
    if (IsKeyPressed(KEY_BACKSPACE)) {
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
  }

  DrawRectangleLinesEx(e->bound, 1, e->color);
  DrawLineEx((Vector2){cursor_pos.x, cursor_pos.y + 5},
             (Vector2){cursor_pos.x, cursor_pos.y + e->bound.height - 5}, 1,
             CURSOR_COLOR);

  DrawText(e->text, e->bound.x, e->bound.y, 14, ENTRY_TEXT_COLOR);
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

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice();

  Button b = create_button();
  Entry e = create_entry();
  bool quit = false;

  // SetTargetFPS(10);

  while (!WindowShouldClose() && !quit) {

    if (IsKeyPressed(KEY_ENTER)) {
      quit = true;
    }

    Vector2 mouse_pos = GetMousePosition();
    BeginDrawing();
    ClearBackground(RAYWHITE);
    if (CheckCollisionPointRec(mouse_pos, b.bound)) {
      b.hover = true;
    } else {
      b.hover = false;
    }

    if (CheckCollisionPointRec(mouse_pos, e.bound)) {
      e.hover = true;
    } else {
      e.hover = false;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      if (b.hover) {
        b.is_pressed = true;
      }
      if (e.hover) {
        e.selected = true;
      } else {
        e.selected = false;
      }

    } else {
      b.is_pressed = false;
    }

    draw_button(b);
    draw_entry(&e);

    DrawFPS(10, 10);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
