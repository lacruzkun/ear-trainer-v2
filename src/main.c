#include <raylib.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Ear Trainer");
  InitAudioDevice();

  while (!WindowShouldClose()) {

    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawRectangle(100, 100, 150, 50, BEIGE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
