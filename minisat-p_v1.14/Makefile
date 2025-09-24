##
##  Makefile for Standard, Profile, Debug, Release, and Release-static versions of MiniSat
##
##    eg: "make rs" for a statically linked release version.
##        "make d"  for a debug version (no optimizations).
##        "make"    for the standard version (optimized, but with debug information and assertions active)

CSRCS     = $(wildcard *.C)
CHDRS     = $(wildcard *.h)
COBJS     = $(addsuffix .o, $(basename $(CSRCS)))

PCOBJS    = $(addsuffix p,  $(COBJS))
DCOBJS    = $(addsuffix d,  $(COBJS))
RCOBJS    = $(addsuffix r,  $(COBJS))

EXEC      = minisat

CXX       = g++ 
CFLAGS    = -Wall -ffloat-store -pthread -std=c++14
#COPTIMIZE = -O6 -m32 -fPIC
#COPTIMIZE = -O3 -m32 -fPIC
ifdef PDT32
# StQ: 32-bit
#COPTIMIZE = -O6 -m32 -g -malign-double -fPIC
#COPTIMIZE = -O0 -m32 -g -pthread -fPIC
COPTIMIZE = -O6 -m32 -g -pthread -fPIE
else
# StQ: 64-bit
COPTIMIZE = -O6 -g -fPIE
COPTIMIZE = -O6 -fPIE
#CFLAGS = -g -Wall -fPIC -DLIN -pthread -ffloat-store -mtune=native -malign-double -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
CFLAGS = -g -Wall -fPIE -DLIN64 -pthread -ffloat-store -mtune=native -malign-double -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4 -std=c++14
#COPTIMIZE =
endif

.PHONY : s p d r build clean depend

s:	WAY=standard
p:	WAY=profile
d:	WAY=debug
r:	WAY=release
rs:	WAY=release static

#s:	CFLAGS+=-g -ggdb -D DEBUG
p:	CFLAGS+=$(COPTIMIZE) -pg -ggdb -D DEBUG
d:	CFLAGS+=-O0 -g -ggdb -D DEBUG
r:	CFLAGS+=$(COPTIMIZE) -D NDEBUG
rs:	CFLAGS+=$(COPTIMIZE) -D NDEBUG

s:	build $(EXEC)
p:	build $(EXEC)_profile
d:	build $(EXEC)_debug
r:	build $(EXEC)_release
rs:	build $(EXEC)_static

build:
	@echo Building $(EXEC) "("$(WAY)")"

clean:
	@rm -f $(EXEC) $(EXEC)_profile $(EXEC)_debug $(EXEC)_release $(EXEC)_static \
	  $(COBJS) $(PCOBJS) $(DCOBJS) $(RCOBJS) libminisat.a
#	  $(COBJS) $(PCOBJS) $(DCOBJS) $(RCOBJS) libminisat.a depend.mak

RANLIB = ranlib
AR = ar
libminisat.a:   $(COBJS)
	@rm -f libminisat.a
	$(AR) cr libminisat.a $(COBJS)
	$(RANLIB) libminisat.a


## Build rule
%.o %.op %.od %.or:	%.C
#	@echo Compiling: $<
	$(CXX) $(CFLAGS) ${COPTIMIZE} -c -o $@ $<

## Linking rules (standard/profile/debug/release)
$(EXEC): $(COBJS) libminisat.a
	@echo Linking $(EXEC)
	@$(CXX) -fPIC $(COBJS) -Wall -o $@ 
#	@$(CXX) -m32 -fPIC $(COBJS) -lz -Wall -o $@ 
#	@$(CXX) -m32 -fPIC $(COBJS) -lz -ggdb -Wall -o $@ 

$(EXEC)_profile: $(PCOBJS)
	@echo Linking $@
	@$(CXX) -m32 -fPIC $(PCOBJS) -lz -ggdb -Wall -pg -o $@ 

$(EXEC)_debug:	$(DCOBJS)
	@echo Linking $@
	@$(CXX) -m32 -fPIC $(DCOBJS) -lz -ggdb -Wall -o $@

$(EXEC)_release: $(RCOBJS)
	@echo Linking $@
	@$(CXX) -m32 -fPIC $(RCOBJS) -lz -Wall -o $@

$(EXEC)_static: $(RCOBJS)
	@echo Linking $@
	@$(CXX) -m32 -fPIC --static $(RCOBJS) -lz -Wall -o $@


## Make dependencies
depend:	depend.mak
depend.mak: $(CSRCS) $(CHDRS)
	@echo Making dependencies ...
	@$(CXX) -MM $(CSRCS) > depend.mak
	@cp depend.mak /tmp/depend.mak.tmp
	@sed "s/o:/op:/" /tmp/depend.mak.tmp >> depend.mak
	@sed "s/o:/od:/" /tmp/depend.mak.tmp >> depend.mak
	@sed "s/o:/or:/" /tmp/depend.mak.tmp >> depend.mak
	@rm /tmp/depend.mak.tmp

include depend.mak
