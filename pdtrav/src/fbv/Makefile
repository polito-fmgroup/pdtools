#
# Compiler option
#
CC = g++
ICFLAGS        = -g -Wall -m32 -fPIC
#ICFLAGS        = -O6 -Wall -m32 -fPIC
LINKER =	g++
#
# SunOS
#XCFLAGS = -DHAVE_IEEE_754 -DUNIX100
#RANLIB = :
#
# Linux
#XCFLAGS = -DHAVE_IEEE_754 -DBSD -DDD_STATS -DDDMP_DEBUG 
XCFLAGS = -DHAVE_IEEE_754 -DBSD -DDD_STATS
XCFLAGS += $(READLINEFLAGS)
RANLIB = ranlib
#
EXE         =
#LDFLAGS     = -static
#PDTRAVFLAGS = -lreadline -ldl -lncurses -lm
LDFLAGS     = -pthread
PDTRAVFLAGS = -lreadline -ldl -lm
#

CFLAGS  = $(ICFLAGS) $(XCFLAGS)

#
# Directories
#

ROOT_PATH   = ../../..
MINISAT     = $(ROOT_PATH)/minisat-p_v1.14
CUDD        = $(ROOT_PATH)/cudd-2.4.1
PDTRAV      = $(ROOT_PATH)/pdtrav-2.9.1
ABC 	    = $(ROOT_PATH)/abc00625p
AIGER       = $(ROOT_PATH)/aiger-20071012

FBV_INCLUDE    = .
PDTRAV_SRC     = $(PDTRAV)/src
PDTRAV_INCLUDE = $(PDTRAV)/include
PDTRAV_LIB     = $(PDTRAV)/lib

MINISAT_INCLUDE = $(MINISAT)
AIGER_INCLUDE = $(AIGER)

.SUFFIXES: .o .c
TARGET  = fbv

SRC     = fbv.c fbvThrd.c fbvAig.c fbvBdd.c fbvMc.c fbvProj.c fbvComb.c fbvAbcLink.c 
HDR     = fbvInt.h fbv.h 

OBJ	= $(SRC:.c=.o)
CUDD_DIR = $(CUDD)
ABC_DIR = $(ABC)
MINISAT_DIR = $(MINISAT)
AIGER_DIR = $(AIGER)

LIBS	= $(PDTRAV_LIB)/pdtlib.a \
          $(MINISAT_DIR)/libminisat.a \
          $(AIGER_DIR)/libaiger.a \
	  $(CUDD_DIR)/dddmp/libdddmp.a \
	  $(CUDD_DIR)/cudd/libcudd.a \
	  $(CUDD_DIR)/mtr/libmtr.a \
	  $(CUDD_DIR)/st/libst.a  \
	  $(CUDD_DIR)/epd/libepd.a  \
	  $(CUDD_DIR)/util/libutil.a \
	  $(ABC_DIR)/libabc.a

$(TARGET): $(SRC) $(OBJ) $(HDR) $(LIBS)
	$(LINKER) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ) $(LIBS) $(PDTRAVFLAGS) 

%.o : %.c $(HDR)
	$(CC) -c $< -I$(PDTRAV_INCLUDE) -I$(MINISAT_INCLUDE) -I$(AIGER_INCLUDE) -I$(FBV_INCLUDE) $(CFLAGS) $(DDDEBUG)

clean:
	rm -f fbv *.o *.u mon.out gmon.out *.pixie *.Addrs *.Counts mnem.* \
	.pure core *.warnings

distclean: clean
	rm -f $(TARGET) *.bak *~ .gdb_history *.qv *.qx

