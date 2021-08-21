SRC_DIR:=../ATSAMF

CC:=gcc
CFLAGS:=\
	-D__AVR_ATmega328P__\
	-DARDUINO=105\
	-DF_CPU=16000000L\
	-I/opt/arduino/hardware/arduino/avr/cores/arduino\
	-I/opt/arduino/hardware/tools/avr/avr/include\
	-I/opt/arduino/hardware/arduino/avr/variants/standard\
	-I$(SRC_DIR)\
	-Wno-attributes\
	-O

test: key.o test_key.o
	cabal clean
	cabal new-test

key.o: $(SRC_DIR)/key.ino .FORCE
	$(CC) $(CFLAGS) -x c++ -c $<

test_key.o: test_key.c .FORCE
	$(CC) $(CFLAGS) -c $<

.FORCE:

.PHONY: .FORCE
