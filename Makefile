OBJS = cpee.o copy.o archive.o
LIBS = -lcrypto -larchive
CFLAGS = -W
CC = gcc

cpee: $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

.c.o:
	$(CC) -c $<

