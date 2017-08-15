
EXEC_OUTPUTNAME = ez_rtp_tester
LIB_OUTPUTNAME  = libezrtp.a

EXEC_DIR = bin
LIB_DIR = lib
OBJ_DIR = obj

CC = gcc
CFLAGS = -Wall
EXEC_LDFLAGS = -Llib -lezrtp
LIB_LDFLAGS = 

src = $(wildcard *.c)
obj = $(src:.c=.o)

$(EXEC_OUTPUTNAME): ez_rtp_tester.c $(LIB_OUTPUTNAME)
	@mkdir -p $(EXEC_DIR)
	$(CC) -o $(EXEC_DIR)/$@ $< $(EXEC_LDFLAGS)

$(LIB_OUTPUTNAME): ez_rtp.o
	@mkdir -p $(LIB_DIR)
	ar rcs $(LIB_DIR)/$@ $(OBJ_DIR)/$<

ez_rtp.o: ez_rtp.c ez_rtp.h	
	@mkdir -p $(OBJ_DIR)
	$(CC) -c -o $(OBJ_DIR)/$@ $<

#ez_rtp_tester.o: ez_rtp_tester.c
#	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(EXEC_DIR)

