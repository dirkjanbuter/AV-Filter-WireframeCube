CC = gcc
SRCS = main.c matrix2d.c matrix3d.c vector2d.c vector3d.c
FILES = $(addprefix src/, $(SRCS))
OBJS = $(FILES:.c=.o)
LIBS = -lm
CFLAGS =  

%.o: %.c $(FILES)
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC 

build: $(OBJS)
	$(CC) $(OBJS) $(LIBS) --shared -o ./../build/filter_wireframecube.so

dist: build
		rm $(OBJS)

clean:
		rm $(OBJS)
