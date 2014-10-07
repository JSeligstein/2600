CC=g++ -m32 -g
CFLAGS=-I/usr/X11/include -I/opt/local/include -L/usr/X11/lib -L/opt/local/lib/ -lm -pthread -lX11
FILES=2600.c tia.cpp

all: 2600


2600: $(FILES)
	$(CC) -o 2600 $(CFLAGS) $(FILES)
	
clean:
	rm -fr *.o 2600
