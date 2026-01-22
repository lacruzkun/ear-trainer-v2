#include <raylib.h>
#include <stdio.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  SetConfigFlags(FLAG_WINDOW_HIGHDPI);
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice();

  bool quit = false;

  while (!WindowShouldClose() && !quit) {

    if (IsKeyPressed(KEY_ENTER)) {
      quit = true;
    }

    // Vector2 mouse_pos = GetMousePosition();
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawFPS(10, 10);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
