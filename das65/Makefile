SRCS=das65.c
OBJS=$(SRCS:.c=.o)
CFLAGS=-g -O0 -Wall -ansi -pedantic
# CFLAGS=-Os -Wall -ansi -pedantic
TARGET=das65

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)
	strip $@
	ls -l $@
	size $@

test: $(TARGET) 6502.bin
	./$(TARGET) 6502.bin > 6502.lst

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)
	$(RM) *.log *.lst

# vim:set ts=2 sw=2 noet:
