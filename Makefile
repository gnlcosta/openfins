
# root directory
ROOT_DIR = $(shell pwd)

# src file
SRC = omronfins.c

# compilation
INCLUDE_DIR = -I$(ROOT_DIR)/include
LDFLAGS = -L$(ROOT_DIR)
CFLAGS = -rdynamic $(INCLUDE_DIR) -Wall -fPIC -D_FILE_OFFSET_BITS=64


all: libopenfins.a test

libopenfins.a: $(SRC:.c=.o)
	ar rcs $@ $(SRC:.c=.o)
	ranlib $@

	
test: omronfins.o
	$(CC) $(CFLAGS) -DMAIN=1 -o test_udp omronfins.c


clean:
	rm -f test_udp *~ */*~ *.o *.a .depend

	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

.depend: $(SRC)
	$(CC) -M $(CFLAGS) $(SRC) > $@


sinclude .depend
