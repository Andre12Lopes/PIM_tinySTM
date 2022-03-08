# Path to root directory
ROOT ?= .

CC = dpu-upmem-dpurte-clang
LD = $(CC)

TM = tiny
SRCDIR = $(ROOT)/src
INCDIR = $(ROOT)/include
LIBDIR = $(ROOT)/lib
TMLIB = $(LIBDIR)/lib$(TM).a

CPPFLAGS += -I$(INCDIR) -I$(SRCDIR)

ifneq ($(CFG),debug)
	CPPFLAGS += -DNDEBUG
endif

LDFLAGS += -L$(LIBDIR) -l$(TM)
LDFLAGS += -lpthread

CPPFLAGS += -U_FORTIFY_SOURCE
CPPFLAGS += -D_REENTRANT

# Debug/optimization flags (optimized by default)
ifeq ($(CFG),debug)
	CFLAGS += -O0 -ggdb3
else
	# For Link Time Optimization, it requires gold linker and clang compiled with --enable-gold
	# CFLAGS += -O4
	# LDFLAGS += -use-gold-plugin
	CFLAGS += -O3
	CFLAGS += -march=native
endif

# Enable all warnings but unsused functions and labels
CFLAGS += -Wall -Wextra -Wno-unused-label -Wno-unused-function

CPPFLAGS += -I$(SRCDIR)

# DEFINES += -DTX_IN_MRAM
# DEFINES += -DOR_IN_MRAM

# DEFINES += -DWRITE_BACK_CTL
DEFINES += -DWRITE_BACK_ETL
# DEFINES += -DWRITE_THROUGH_ETL

.PHONY:	all test clean

all: $(TMLIB)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFINES) -c -o $@ $<

# Additional dependencies
$(SRCDIR)/tiny.o: $(SRCDIR)/tiny.h $(SRCDIR)/tiny_internal.h $(SRCDIR)/tiny_wtetl.h $(SRCDIR)/utils.h $(SRCDIR)/atomic.h


$(TMLIB): $(SRCDIR)/$(TM).o
	$(AR) crus $@ $^

test: $(TMLIB)
	$(MAKE) -C test

clean:
	rm -f $(TMLIB) $(SRCDIR)/*.o
	TARGET=clean $(MAKE) -C test


# gcc -I/home/andre/Documents/PIM_tinySTM/src -o prog main.c -L/home/andre/Documents/PIM_tinySTM/lib/ -l tiny