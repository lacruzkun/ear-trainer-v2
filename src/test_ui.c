#include <raylib.h>
#include <stdio.h>
#include <ui.c>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice();

  Button b = create_button();
  Entry e = create_entry();
  bool quit = false;

  SetTargetFPS(FPS);

  while (!WindowShouldClose() && !quit) {

    if (IsKeyPressed(KEY_ENTER)) {
      quit = true;
    }

    Vector2 mouse_pos = GetMousePosition();
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);
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
