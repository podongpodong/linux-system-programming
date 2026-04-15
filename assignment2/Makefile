CFLAGS = -g

ssu_cleanup : ssu_cleanupd.o help.o add.o parse_option.o show.o dremove.o arrange.o modify.o
	gcc $(CFLAGS) ssu_cleanupd.o help.o add.o parse_option.o show.o dremove.o arrange.o modify.o -o ssu_cleanupd

ssu_cleanup.o: ssu_cleanupd.c
	gcc $(CFLAGS) -c ssu_cleanupd.c

help.o: help.c
	gcc $(CFLAGS) -c help.c

add.o: add.c
	gcc $(CFLAGS) -c add.c

parse_option.o: parse_option.c
	gcc $(CFLAGS) -c parse_option.c

show.o: show.c
	gcc $(CFLAGS) -c show.c

dremove.o: dremove.c
	gcc $(CFLAGS) -c dremove.c

arrange.o: arrange.c
	gcc $(CFLAGS) -c arrange.c

modify.o: modify.c
	gcc $(CFLAGS) -c modify.c

clean :
	rm ssu_cleanupd.o
	rm help.o
	rm add.o
	rm parse_option.o
	rm show.o
	rm dremove.o
	rm arrange.o
	rm modify.o
	rm ssu_cleanupd
