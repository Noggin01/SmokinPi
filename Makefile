CC=gcc
CFLAGS=-I.
IDIR=.
ODIR=./obj
LIBS=-lpigpio -lpthread -lrt -lncurses

_DEPS = app.h main.h rev_history.h thermistor.h cmd_line.h logging.h pid.h servo.h tlc1543.h eth_comms.h monitor.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = app.o logging.o main.o servo.o thermistor.o tlc1543.o cmd_line.o pid.o eth_comms.o monitor.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

smokinpi: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
