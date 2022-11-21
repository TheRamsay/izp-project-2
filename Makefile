CC=gcc
CFLAGS = -std=c99 -Wall -Wextra -ggdb -fno-omit-frame-pointer -fsanitize=address -O1

cluster: cluster.o
	$(CC) -o cluster cluster.c -lm

