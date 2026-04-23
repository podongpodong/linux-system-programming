CC = gcc
CFLAGS =  -Wall -Wextra -Wpedantic -g
TARGET = ssu_ext2
SRCS = main.c util.c ssu_ext2.c help.c tree.c print.c 
OBJS = $(SRCS:.c=.o)

#all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c	
	$(CC) $(CFLAGS) -c $< -o $@

main.o: ssu_ext2.h util.h
ssu_ext2.o: ext2_struct.h ssu_ext2.h
tree.o: ext2_struct.h ssu_ext2.h util.h
print.o: util.h
help.o: util.h

clean:
	rm -f $(TARGET) $(OBJS)

run: $(TARGET)
	./$(TARGET)
