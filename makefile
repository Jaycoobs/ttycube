CC=cc
CFLAGS=-I /usr/include/ -L /usr/lib/ -l ncurses -l m

cube: cube.c
	$(CC) -o out/cube cube.c $(CFLAGS)
