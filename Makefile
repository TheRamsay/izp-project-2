CC=gcc
CFLAGS = -std=c99 -Wall -Wextra -ggdb 

cluster: cluster.o
	$(CC) -o cluster cluster.c -lm

