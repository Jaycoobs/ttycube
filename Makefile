CC=cc
CFLAGS=-lm

ttycube: cube.c
	$(CC) $(CFLAGS) $^ -o $@ 
