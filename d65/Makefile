CFLAGS = -Wall -ansi -pedantic -Os
SRCS = d6502.c
OBJS = $(SRCS:.c=.o)

all: d65

d65: $(OBJS)
	$(CC) -o $@ $(OBJS)
	strip $@

clean:
	$(RM) $(OBJS)
	$(RM) d65
	$(RM) *.log

# vim:set ts=2 sw=2 noet:
