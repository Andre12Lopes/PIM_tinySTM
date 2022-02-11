ROOT = ../..

CC = dpu-upmem-dpurte-clang
NR_TASKLETS = 10

TM = tiny
SRCDIR = $(ROOT)/src
INCDIR = $(ROOT)/include
LIBDIR = $(ROOT)/lib
TMLIB = $(LIBDIR)/lib$(TM).a
TARGET = bank

CFLAGS += -Wall
CFLAGS += -Wno-unused-label -Wno-unused-function -std=c11

LDFLAGS += -L$(LIBDIR) -l$(TM)
CPPFLAGS += -I$(SRCDIR)

.PHONY:	all clean

all: $(TARGET)

%.o: %.c
	$(CC) -DNR_TASKLETS=$(NR_TASKLETS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TARGET): %: %.o $(TMLIB)
	$(CC) -o $@ $< -DNR_TASKLETS=$(NR_TASKLETS) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o