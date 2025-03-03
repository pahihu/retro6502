SRCS=das65.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-g -O0 -Wall -ansi -pedantic
TARGET=das65

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)
	# strip $@
	size $@

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(RM) *.log

# vim:set ts=2 sw=2 noet:
