CFLAGS = -Wall  -ggdb  
LIBS = -lraylib -lGL -lm -lpthread
SRC_DIR = include
INC_SRC = $(wildcard  $(SRC_DIR)/*.c)
CC = clang

ear_trainer: src/main.c
	$(CC) $(CFLAGS) -Llib -Iinclude $(INC_SRC) src/main.c -o build/ear_trainer $(LIBS)
