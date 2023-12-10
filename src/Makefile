# https://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR =../lib

LIBS=-lm

_DEPS = distributed_directory.h interconnect.h queue.h single_cache.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = distributed_directory.o interconnect.o queue.o single_cache.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hellomake: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 