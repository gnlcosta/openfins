
# root directory
ROOT_DIR = $(shell pwd)

# src file
SRC = omronfins.c

# compilation
INCLUDE_DIR = -I$(ROOT_DIR)/include
LDFLAGS = -L$(ROOT_DIR)
CFLAGS = $(INCLUDE_DIR) -Wall


all: libopenfins.a

libopenfins.a: $(SRC:.c=.o)
	ar rcs $@ $(SRC:.c=.o)
	ranlib $@


clean:
	rm -f *~ */*~ *.o *.a .depend

	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

.depend: $(SRC)
	$(CC) -M $(CFLAGS) $(SRC) > $@


sinclude .depend
