CC=gcc
CFLAGS = -std=c99 -Wall -Wextra -g

cluster: cluster.o
	$(CC) $(CFLAGS) -o cluster cluster.c -lm

