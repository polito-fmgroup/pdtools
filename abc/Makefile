CC   := gcc
CXX  := g++
LD   := g++
CP   := cp

PROG := abc

MODULES := \
        $(wildcard src/ext) src/misc/ext \
	src/base/abc src/base/abci src/base/cmd src/base/io \
	src/base/main src/base/ver src/base/test \
	src/bdd/cudd src/bdd/dsd src/bdd/epd src/bdd/mtr src/bdd/parse \
	src/bdd/reo src/bdd/cas \
	src/map/mapper src/map/mio src/map/super src/map/if \
	src/map/amap src/map/cov src/map/scl \
	src/misc/extra src/misc/mvc src/misc/st src/misc/util src/misc/nm \
	src/misc/vec src/misc/hash src/misc/tim src/misc/bzlib src/misc/zlib \
	src/misc/mem src/misc/bar src/misc/bbl \
	src/opt/cut src/opt/fxu src/opt/rwr src/opt/mfs src/opt/sim \
	src/opt/ret src/opt/res src/opt/lpk src/opt/nwk src/opt/rwt \
	src/opt/cgt src/opt/csw src/opt/dar src/opt/dau \
	src/sat/bsat src/sat/csat src/sat/msat src/sat/psat src/sat/cnf src/sat/bmc \
	src/bool/bdc src/bool/deco src/bool/dec src/bool/kit src/bool/lucky \
	src/proof/pdr src/proof/int src/proof/bbr src/proof/llb src/proof/live \
	src/proof/cec src/proof/dch src/proof/fraig src/proof/fra src/proof/ssw \
	src/proof/abs \
	src/aig/aig src/aig/saig src/aig/gia src/aig/ioa src/aig/ivy src/aig/hop \
	src/aig/miniaig \
	src/python 

abc_dir		= ../abc
ABC_DIR 	= $(shell cd $(abc_dir);pwd)

all: $(PROG)
default: $(PROG)

# Please note that to compile on 32-bit Linux the following optflags are required:
# -DLIN -DSIZEOF_VOID_P=4 -DSIZEOF_LONG=4 

#OPTFLAGS  := -DNDEBUG -O3 -DLIN
#OPTFLAGS  := -DNDEBUG -O3 -DLIN64

ifdef PDT32
# StQ: 32-bit
OPTFLAGS  := -O3 -DLIN -m32 -pthread -fPIE -DABC_NAMESPACE=xxx
#OPTFLAGS  := -g -O -DLIN -m32
else
# StQ: 64 bit = Original Makefile Version
#OPTFLAGS  := -g -O -DLIN
#OPTFLAGS  := -g -O -DLIN64 -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4 -DABC_NAMESPACE=xxx
OPTFLAGS  := -g -Wall -fPIE -DLIN64 -pthread -ffloat-store -mtune=native -malign-double -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4 -DABC_NAMESPACE=xxx
endif

arch_flags : arch_flags.c
	$(CC) arch_flags.c -o arch_flags

ARCHFLAGS := $(shell $(CC) arch_flags.c -o arch_flags && ./arch_flags)
OPTFLAGS  := -g -O3 #-DABC_NAMESPACE=xxx

#CFLAGS   += -Wall -Wno-unused-function -Wno-unused-but-set-variable $(OPTFLAGS) $(patsubst %, -I%, $(MODULES)) 
#CFLAGS   += -Wall -Wno-unused-function -Wno-unused-but-set-variable $(OPTFLAGS) $(ARCHFLAGS) -I$(PWD)/src
CFLAGS   += -Wall -fPIE -Wno-unused-function $(OPTFLAGS) $(ARCHFLAGS) -I$(ABC_DIR)/src
CXXFLAGS += $(CFLAGS) 

LIBS := -ldl -lreadline -lpthread -lncurses
#LIBS := -lreadline

SRC  := 
GARBAGE := core core.* *.stackdump ./tags $(PROG)

.PHONY: tags clean docs

include $(patsubst %, %/module.make, $(MODULES))

OBJ := \
	$(patsubst %.cc, %.o, $(filter %.cc, $(SRC))) \
	$(patsubst %.c, %.o,  $(filter %.c, $(SRC)))  \
	$(patsubst %.y, %.o,  $(filter %.y, $(SRC))) 

DEP := $(OBJ:.o=.d)

# implicit rules

%.d: %.c
	@echo "\`\` Dependency:" $(LOCAL_PATH)/$<
	@./depends.sh $(CC) `dirname $*.c` $(CFLAGS) $*.c > $@

%.d: %.cc
	@./depends.sh $(CXX) `dirname $*.cc` $(CXXFLAGS) $(CFLAGS) $*.cc > $@

-include $(DEP)

clean: 
	@echo "\`\` Cleaning up..."
	@rm -rvf $(PROG) lib$(PROG).a $(OBJ) $(GARBAGE) $(OBJ:.o=.d) 

tags:
  # etags `find . -type f -regex '.*\.\(c\|h\)'`
	ctags -R .

$(PROG): $(OBJ)
	$(LD) -o $@ $^ $(LIBS)

lib$(PROG).a: $(OBJ)
	ar rv $@ $?
	ranlib $@

docs:
	doxygen doxygen.conf
