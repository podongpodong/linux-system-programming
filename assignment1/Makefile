CC = gcc
CFLAGS =  -Wall -Wextra -Wpedantic -g
TARGET = ssu_cleanup
SRCS = ssu_cleanup.c util.c help.c tree.c arrange.c hashtable.c
OBJS = $(SRCS:.c=.o)

#all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@

ssu_cleanup.o util.o tree.o arrange.o help.o: util.h
arrange.o: hashtable.h

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)
