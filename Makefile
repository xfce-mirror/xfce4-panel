OBJS = xfce.o central.o side.o \
       popup.o item.o module.o settings.o icons.o wmhints.o \
       builtins.o move.o dialogs.o callbacks.o
PROG = xfce2


CFLAGS = -Wall -DGTK_DISABLE_DEPRECATED -g

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 gmodule-2.0 libxml-2.0`
GTK_LIBS = `pkg-config --libs gtk+-2.0 gmodule-2.0 libxml-2.0`

COMPILE = $(CC) $(CFLAGS) $(GTK_CFLAGS) -c

subdirs = plugins

all: $(PROG) sub

$(PROG): $(OBJS)
	$(CC) $(GTK_LIBS) -o $(PROG) $(OBJS)

mailcheck.o:
	$(CC) -fPIC $(GTK_CFLAGS) -c mailcheck.c -o mailcheck.o

sub:
	(cd plugins; make)
	
%.o: %.c %.h xfce.h
	$(COMPILE) -o $@ $<

clean:
	-rm $(OBJS) $(PROG) 
	(cd plugins; make clean)
	
