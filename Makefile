CFLAGS+=-std=c99 -g -I decs/
LDFLAGS+=-lSDL2 -lSDL2_ttf -lm
OBJS+= ttf.o

include decs/Makefile.include

all: particle

particle: particle.o $(OBJS)

clean:
	rm -f $(OBJS) particle
