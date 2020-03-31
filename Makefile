SRC_FILES = main.c
CC_FLAGS = -Wall -Wextra -g -std=c11
CC = gcc

all:
	mkdir build || ${CC} ${SRC_FILES} ${CC_FLAGS} -o ./build/mac && clear && ./build/mac /Users/emran/Desktop/lc3-vm/disk.img