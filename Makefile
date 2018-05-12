CFLAGS+=-O2 -std=c99 -Wall -Wno-missing-braces -g -I decs/
CFLAGS+=`pkg-config --cflags sdl2`
LDFLAGS+=-lSDL2 -lSDL2_ttf -lGL -lGLEW -lm
OBJS+= phys.o ttf.o shader.o phys_sphere_col.o

include decs/Makefile.include

all: particle

depend: .depend

.depend: $(OBJS:.o=.c) particle.c
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^ > ./.depend;

include .depend

particle: particle.o $(OBJS)

clean:
	rm -f ./.depend
	rm -f $(OBJS) particle.o particle
