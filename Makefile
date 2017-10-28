CFLAGS+=-std=c99 -g -I decs/
LDFLAGS+=-lSDL2 -lSDL2_ttf -lm
OBJS+= phys.o ttf.o

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
