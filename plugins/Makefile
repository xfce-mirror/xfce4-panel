CC = gcc
CFLAGS = -Wall -g

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0`
GTK_LIBS = `pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0`

all: mailcheck

$(PROG): $(OBJS)
	$(CC) $(GTK_LIBS) -o $(PROG) $(OBJS)

mailcheck.o: mailcheck.c
	$(CC) -I. -I.. -fPIC $(GTK_CFLAGS) -c mailcheck.c -o mailcheck.o

mailcheck: mailcheck.o
	  $(CC) $(GTK_LIBS) -shared -Wl,-soname,libmailcheck.so.1 \
	  -o libmailcheck.so mailcheck.o
	
clean:
	-rm mailcheck.o *.so*
	
