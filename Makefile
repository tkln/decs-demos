CFLAGS+=-std=c99 -g -I decs/
LDFLAGS+=-lSDL2 -lSDL2_ttf -lm
OBJS+= game.o ttf.o

include decs/Makefile.include

all: game

game: $(OBJS)

clean:
	rm -f $(OBJS) game
