CC=gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -DNDEBUG 

cluster: cluster.o
	$(CC) $(CFLAGS) -o cluster cluster.c -lm

