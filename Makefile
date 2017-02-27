CFLAGS=-Wall -g

HEADER=proj3.h
SOURCE_MAIN=oss.c
SOURCE_SLAVE=slave.c
OBJECT_MAIN=oss.o
OBJECT_SLAVE=slave.o
EXE_MAIN=oss
EXE_SLAVE=user

all: $(EXE_MAIN) $(EXE_SLAVE)

$(EXE_MAIN): $(OBJECT_MAIN)
	gcc $(OBJECT_MAIN) -o $(EXE_MAIN)

$(EXE_SLAVE): $(OBJECT_SLAVE)
	gcc $(OBJECT_SLAVE) -o $(EXE_SLAVE)

%.o: %.c $(HEADER)
	gcc -c $(CFLAGS) $*.c -o $*.o

.PHONY: clean

clean:
	rm *.o $(EXE_MAIN) $(EXE_SLAVE)




