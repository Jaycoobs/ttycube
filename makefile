CC=cc
CFLAGS=-I /usr/include/ -L /usr/lib/ -l ncurses -l m

ttycube: cube.c
	$(CC) -o ttycube cube.c $(CFLAGS)

install: ttycube
	install -Dm755 ttycube $1/usr/bin/ttycube
