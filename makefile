CFLAGS = -Wall  -ggdb  -Wextra
LIBS = -lraylib -lGL -lm -lpthread
SRC_DIR = include
INC_SRC = $(wildcard  $(SRC_DIR)/*.c)
CC = clang

ear_trainer: src/main.c
	$(CC) $(CFLAGS) -Llib -Iinclude src/main.c -o build/ear_trainer $(LIBS)

test_ui: src/test_ui.c
	$(CC) $(CFLAGS) -Llib -Iinclude src/test_ui.c -o build/test_ui $(LIBS)

test_soundfont: src/test_soundfont.c
	$(CC) $(CFLAGS) -Llib -Iinclude src/test_soundfont.c -o build/test_soundfont $(LIBS)

test_render: src/test_render.c
	$(CC) $(CFLAGS) -Llib -Iinclude src/test_render.c -o build/test_render $(LIBS)
