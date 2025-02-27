CFLAGS = -Wall -ansi -pedantic -Os
SRCS = a65.c
OBJS = $(SRCS:.c=.o)

all: a65

a65: $(OBJS)
	$(CC) -o $@ $(OBJS)
	strip $@

clean:
	$(RM) $(OBJS)
	$(RM) a65

# vim:set ts=3 sw=3 noet:
