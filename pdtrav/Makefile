#------------------------------------------#
# Makefile for the PdTRAV distribution kit #
# Version 3.5.0                            #
#------------------------------------------#

# Targets:
#   all (default)  build the external libraries, the pdtrav library and the
#                  pdtrav and pdtapp executables
#   default, static, sanitize
#                  same as all, linking dynamically (default), statically,
#                  or with AddressSanitizer
#   clean          remove pdtrav's build output
#   distclean      remove the build output of pdtrav and of all the
#                  external libraries
#
# The top-level Makefile normally drives this one and supplies the compilers
# and flags from ../config.mk. This Makefile can also be run on its own.

SHELL = /bin/sh

# Compilers and flags shared with the top-level build. The include is
# optional: without it, the fallback values below apply.
-include ../config.mk

.SUFFIXES:
#----------------------------------------#
# Configuration                          #
#----------------------------------------#

.PHONY : default static 

default: LDFLAGS = 
static : LDFLAGS = -static
sanitize : LDFLAGS = -fsanitize=address -static-libasan
sanitize : SANITIZER = -fsanitize=address

default: all
static : all
sanitize : all

# PDT32=yes selects a 32-bit build. DEFINE_PDT32 passes the setting on to the
# sub-builds that support it.
ifdef PDT32
DEFINE_PDT32 = PDT32=yes
else
DEFINE_PDT32 =
endif

# DEBUG=1 selects the debug builds of MiniSat 2.2 and Lingeling. The top-level
# build selects them through BUILD_MODE instead.
ifdef DEBUG
MINISAT22LIB = minisat22d
LGL_FLAGS = CFLAGS="$(LGLDBGCFLAGS)"
else
MINISAT22LIB = minisat22
LGL_FLAGS = CFLAGS="$(LGLNDBGCFLAGS)"
endif

# Compilers and compilation flags.
#   CC          C++ compiler for pdtrav's sources, also used to link (the
#               name is historical)
#   CUDD_CC     C compiler for CUDD
# Both come from config.mk and default to g++ and gcc without it.
# CUDD_ICFLAGS is passed to CUDD only; CUDD_XCFLAGS is passed to CUDD and is
# also added to pdtrav's CFLAGS below.
CC :=		$(if $(TOOLCHAIN_CXX),$(TOOLCHAIN_CXX),g++)
CUDD_CC :=	$(if $(TOOLCHAIN_CC),$(TOOLCHAIN_CC),gcc)

LGLDBGCFLAGS =	-Wall -g -DNLGLOG -DNCHKSOL -DNLGLDRUPLIG -DNLGLYALSAT -DNLGLFILES -DNLGLDEMA 
LGLNDBGCFLAGS =	$(if $(LGL_BUILD_FLAGS),$(LGL_BUILD_FLAGS),-Wall -O6 -DNLGLOG -DNCHKSOL -DNLGLDRUPLIG -DNLGLYALSAT -DNLGLFILES -DNLGLDEMA)


ifdef PDT32
# 32-bit build. Worked with CUDD 2.4.1, and with CUDD 2.4.2 when CUDD is
# compiled with gcc rather than g++.
CUDD_ICFLAGS = 	-O6 -m32 -Wall -pthread
CUDD_XCFLAGS =	-mtune=native -fPIC -DHAVE_IEEE_754 -DBSD
XC_FLAGS =
CFLAGS =	-DLIN -g -m32 -pthread
else
# 64-bit build. CUDD_ICFLAGS and CUDD_XCFLAGS come from config.mk when it is
# included; the values below are the fallback for running this Makefile on its
# own. Host-specific flags such as -mtune=native and -malign-double are not
# part of the defaults; see ARCH_FLAGS in config.mk.
CUDD_ICFLAGS ?= 	-O6 -Wall -fPIE -pthread -std=c++14
CUDD_XCFLAGS ?=	-ffloat-store -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4
XC_FLAGS =	-DHAVE_IEEE_754=1 -DBSD=1 -DSTDC_HEADERS=1 -DHAVE_STRSTR=1
# Optimisation and debug flags come from OPT_FLAGS in config.mk (set by
# BUILD_MODE). Without config.mk, PDTOPT=1 selects -O6 and the default is -g.
ifdef OPT_FLAGS
CFLAGS =	-DLIN64 $(OPT_FLAGS) -Wall -fPIE -pthread $(SANITIZER) -Wno-error=implicit-function-declaration
else
ifdef PDTOPT
CFLAGS =	-DLIN64 -O6 -Wall -fPIE -pthread $(SANITIZER) -Wno-error=implicit-function-declaration
else
CFLAGS =	-DLIN64 -g -Wall -fPIE -pthread $(SANITIZER) -Wno-error=implicit-function-declaration
endif
endif
endif
LINKER =	$(CC)

CFLAGS += 	$(CUDD_XCFLAGS) -std=c++14
OTHERLIBS =	-lreadline -lncurses -ltermcap -ldl -lrt -lm
YACC =		bison -y
LEX =		flex
NAWK =		gawk
AR =		ar
RANLIB =	ranlib
RM =		rm -f
FIND =		find

# -D_GLIBCXX_DEBUG enables libstdc++'s checked containers in every build mode.
ALLCFLAGS = $(CFLAGS) $(XC_FLAGS) -D_GLIBCXX_DEBUG

#-------------#
# Directories #
#-------------#

pdtools_dir 	= ..
PDTOOLS_DIR 	= $(shell cd $(pdtools_dir);pwd)

pdtrav_dir 	= .
PDTRAV_DIR 	= $(shell cd $(pdtrav_dir);pwd)

app_dir 	= ./src/app
APP_DIR 	= $(shell cd $(app_dir);pwd)

fbv_dir 	= ./src/fbv
FBV_DIR 	= $(shell cd $(fbv_dir);pwd)

cudd_dir	= ../cudd-2.5.0
CUDD_DIR 	= $(shell cd $(cudd_dir);pwd)

minisat_dir	= ../minisat-p_v1.14
MINISAT_DIR 	= $(shell cd $(minisat_dir);pwd)

minisat22_dir	= ../minisat-2.2.0-drup
MINISAT22_DIR 	= $(shell cd $(minisat22_dir);pwd)

abc_dir		= ../abc
ABC_DIR 	= $(shell cd $(abc_dir);pwd)

aiger_dir	= ../aiger-1.9.3
AIGER_DIR 	= $(shell cd $(aiger_dir);pwd)

lgl_dir		= ../lingeling-bcj
LGL_DIR 	= $(shell cd $(lgl_dir);pwd)

# Directory where object files will be placed during the build
objectdir 	= $(PDTRAV_DIR)/obj

# Directory where libraries will be placed during the build
libdir 		= $(PDTRAV_DIR)/lib

# Directory where executable files will be placed during the build
bindir 		= $(PDTRAV_DIR)/bin

# Directory where header files will be placed during the build
includedir 	= $(PDTRAV_DIR)/include

#-------------------------#
# Library paths and names #
#-------------------------#

PDTLIB = $(libdir)/pdtlib.a

MNEMLIB	= $(CUDD_DIR)/mnemosyne/libmnem.a

cuddlib = $(CUDD_DIR)/dddmp/libdddmp.a \
	$(CUDD_DIR)/cudd/libcudd.a \
	$(CUDD_DIR)/mtr/libmtr.a \
	$(CUDD_DIR)/st/libst.a  \
	$(CUDD_DIR)/util/libutil.a \
	$(CUDD_DIR)/epd/libepd.a

aigerlib = $(AIGER_DIR)/libaiger.a

lgllib = $(LGL_DIR)/liblgl.a

minisatlib = $(MINISAT_DIR)/libminisat.a

minisat22lib = $(MINISAT22_DIR)/simp/lib.a

abclib = $(ABC_DIR)/libabc.a

LIBS = $(cuddlib) $(aigerlib) $(lgllib) $(minisatlib) $(minisat22lib) $(abclib)

#-----------------------------------------#
# The name of the product and its version #
#-----------------------------------------#

PRODUCT = PdTRAV
VERSION = 3.5.0
EXE = pdtrav
APP = pdtapp

# Compile and version information
#
# COMPILE_DATE and PDTRAV_VERSION are strings surrounded by double
# quotes that contain spaces, e.g., "PdTRAV 2.5"

VERDATE := -DCOMPILE_DATE="\"$(shell date)\"" -DPDTRAV_VERSION="\"$(PRODUCT) $(VERSION)\""

#---------------------------------#
# The list of packages to compile #
#---------------------------------#

PKGS = pdtutil cuplus ddi fsm mc part tr trav expr baig

EXESRC = $(FBV_DIR)/fbv.c \
	$(FBV_DIR)/fbvThrd.c \
	$(FBV_DIR)/fbvAig.c \
	$(FBV_DIR)/fbvBdd.c \
	$(FBV_DIR)/fbvMc.c \
	$(FBV_DIR)/fbvProj.c \
	$(FBV_DIR)/fbvComb.c \
	$(FBV_DIR)/fbvAbcLink.c

EXEHDR = $(FBV_DIR)/fbv.h  $(FBV_DIR)/fbvInt.h

APPSRC = $(APP_DIR)/app.c \
	$(APP_DIR)/appMgr.c \
	$(APP_DIR)/appAiger.c \
	$(APP_DIR)/appCertify.c \
	$(APP_DIR)/appGfp.c \
	$(APP_DIR)/appIso.c

APPHDR = $(APP_DIR)/app.h  $(APP_DIR)/appInt.h

#--------------------------------------------------#
# Include the make templates from all the packages #
#--------------------------------------------------#

MAKEINCLUDES = $(foreach package, $(PKGS), \
	$(PDTRAV_DIR)/src/$(package)/$(package).make)

include $(MAKEINCLUDES)

OBJECTS = $(addprefix $(objectdir)/,$(GENERATEDCSRC:.c=.o) $(CSRC:.c=.o))
EXEOBJ = $(EXESRC:.c=.o)
APPOBJ = $(APPSRC:.c=.o)

#--------------------------------------------------------------#
# Form the list of directories to search for header files.     #
#--------------------------------------------------------------#

VPATH = $(PDTRAV_DIR): \
	$(objectdir): \
	$(addprefix :$(PDTRAV_DIR)/src/,$(PKGS))

INCLUDEDIRS = -I$(includedir) -I$(includedir)/minisat22

#----------------#
# Implicit rules #
#----------------#

# For compiling a source file into the object directory

$(objectdir)/%.o : %.c
	umask 2 ; $(CC) -c $(ALLCFLAGS) $(INCLUDEDIRS) -o $@ $<

%.o : %.c $(EXEHDR)
	umask 2 ; $(CC) -c $(ALLCFLAGS) $(VERDATE) $(INCLUDEDIRS) -o $@ $<

# Place object files into an archive

%.a :<
	rm -f $@
	umask 2; $(AR) cq $@ $^
	$(RANLIB) $@

#----------------------------------#
# Rules for generating directories #
#----------------------------------#

$(objectdir) :
	- umask 2 ; mkdir $(objectdir)

$(libdir) :
	- umask 2 ; mkdir $(libdir)

$(bindir) :
	- umask 2 ; mkdir $(bindir)

$(includedir) :
	- umask 2 ; mkdir $(includedir) ; mkdir -p $(includedir)/misc/vec ; mkdir -p $(includedir)/misc/util ; mkdir -p $(includedir)/misc/extra ; mkdir -p $(includedir)/misc/mem ; mkdir -p $(includedir)/misc/nm ; mkdir -p $(includedir)/misc/st ; mkdir -p $(includedir)/aig/hop ; mkdir -p $(includedir)/aig/saig ; mkdir -p $(includedir)/aig/aig ; mkdir -p $(includedir)/aig/gia ; mkdir -p $(includedir)/sat/cnf ; mkdir -p $(includedir)/opt/dar ; mkdir -p $(includedir)/base/abc ; mkdir -p $(includedir)/base/io ; mkdir -p $(includedir)/base/cmd ; mkdir -p $(includedir)/sat/bsat ; mkdir -p $(includedir)/proof/int ; mkdir -p $(includedir)/base/main ; mkdir -p $(includedir)/proof/ssw ; mkdir -p $(includedir)/minisat22 ; mkdir -p $(includedir)/minisat22/coreDir ; mkdir -p $(includedir)/minisat22/simp ; mkdir -p $(includedir)/minisat22/mtl ; mkdir -p $(includedir)/minisat22/utils ;
	ln -s $(CUDD_DIR)/include/*.h $(includedir)
	ln -s $(AIGER_DIR)/*.h $(includedir)
	ln -s $(LGL_DIR)/*.h $(includedir)
	ln -s $(MINISAT_DIR)/*.h $(includedir)
	ln -s $(MINISAT22_DIR)/coreDir/*.h $(includedir)/minisat22/coreDir
	ln -s $(MINISAT22_DIR)/simp/*.h $(includedir)/minisat22/simp
	ln -s $(MINISAT22_DIR)/mtl/*.h $(includedir)/minisat22/mtl
	ln -s $(MINISAT22_DIR)/utils/*.h $(includedir)/minisat22/utils
	ln -s $(ABC_DIR)/src/proof/int/*.h $(includedir)
	ln -s $(ABC_DIR)/src/proof/ssw/*.h $(includedir)
	ln -s $(ABC_DIR)/src/aig/saig/*.h $(includedir)
	ln -s $(ABC_DIR)/src/sat/cnf/*.h $(includedir)
	ln -s $(ABC_DIR)/src/aig/aig/*.h $(includedir)
	ln -s $(ABC_DIR)/src/aig/hop/*.h $(includedir)
	ln -s $(ABC_DIR)/src/aig/gia/*.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/extra/*.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/vec/*.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/mem/*.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/nm/*.h $(includedir)
	ln -s $(ABC_DIR)/src/opt/dar/*.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/util/abc_global.h $(includedir)
	ln -s $(ABC_DIR)/src/misc/util/utilCex.h $(includedir)
	ln -s $(ABC_DIR)/src/sat/bsat/*.h $(includedir)
	ln -s $(ABC_DIR)/src/base/abc/abc.h $(includedir)
	ln -s $(ABC_DIR)/src/base/abc/*.h $(includedir)/base/abc
	ln -s $(ABC_DIR)/src/base/cmd/*.h $(includedir)/base/cmd
	ln -s $(ABC_DIR)/src/base/io/*.h $(includedir)/base/io
	ln -s $(ABC_DIR)/src/base/main/*.h $(includedir)
	ln -s $(ABC_DIR)/src/base/main/*.h $(includedir)/base/main
	ln -s $(ABC_DIR)/src/proof/int/*.h $(includedir)/proof/int
	ln -s $(ABC_DIR)/src/proof/ssw/*.h $(includedir)/proof/ssw
	ln -s $(ABC_DIR)/src/aig/saig/*.h $(includedir)/aig/saig
	ln -s $(ABC_DIR)/src/sat/cnf/*.h $(includedir)/sat/cnf
	ln -s $(ABC_DIR)/src/aig/aig/*.h $(includedir)/aig/aig
	ln -s $(ABC_DIR)/src/aig/hop/*.h $(includedir)/aig/hop
	ln -s $(ABC_DIR)/src/aig/gia/*.h $(includedir)/aig/gia
	ln -s $(ABC_DIR)/src/misc/extra/*.h $(includedir)/misc/extra
	ln -s $(ABC_DIR)/src/misc/vec/*.h $(includedir)/misc/vec
	ln -s $(ABC_DIR)/src/misc/mem/*.h $(includedir)/misc/mem
	ln -s $(ABC_DIR)/src/misc/nm/*.h $(includedir)/misc/nm
	ln -s $(ABC_DIR)/src/misc/st/*.h $(includedir)/misc/st
	ln -s $(ABC_DIR)/src/opt/dar/*.h $(includedir)/opt/dar
	ln -s $(ABC_DIR)/src/misc/util/abc_global.h $(includedir)/misc/util
	ln -s $(ABC_DIR)/src/misc/util/utilCex.h $(includedir)/misc/util
	ln -s $(ABC_DIR)/src/sat/bsat/*.h $(includedir)/sat/bsat
	@for dir in $(PKGS); do \
		(ln -s $(PDTRAV_DIR)/src/$$dir/*.h $(includedir); )\
	done
	ln -s $(FBV_DIR)/*.h $(includedir)
	ln -s $(APP_DIR)/*.h $(includedir)

#------------------------------------------------------#
# Rules to compile and build libraries and executables #
#------------------------------------------------------#

$(PDTLIB): $(OBJECTS)
	$(AR) cr $@ $?
	$(RANLIB) $@

cudd:
	${MAKE} CC=$(CUDD_CC) RANLIB=$(RANLIB) ICFLAGS="$(CUDD_ICFLAGS)" XCFLAGS="$(CUDD_XCFLAGS)" -C ${CUDD_DIR} cuddlibs

abc:
	${MAKE} -C ${ABC_DIR} $(DEFINE_PDT32) CC=$(or $(TOOLCHAIN_CC),gcc) CXX=$(or $(TOOLCHAIN_CXX),g++) LD=$(or $(TOOLCHAIN_CXX),g++) libabc.a

aiger:
	@if [ -e ${AIGER_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring aiger"; cd ${AIGER_DIR}; CC=$(or $(TOOLCHAIN_CC),gcc) ./configure $(DEFINE_PDT32)) \
	fi
	${MAKE} -C ${AIGER_DIR} CC=$(or $(TOOLCHAIN_CC),gcc) libaiger.a

lgl:
	@if [ -e ${LGL_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring lgl"; cd ${LGL_DIR}; CC=$(or $(TOOLCHAIN_CC),gcc) ./configure.sh $(DEFINE_PDT32)) \
	fi

	${MAKE} $(LGL_FLAGS) -C ${LGL_DIR} CC=$(or $(TOOLCHAIN_CC),gcc) liblgl.a

minisat:
	${MAKE} -C ${MINISAT_DIR} $(DEFINE_PDT32) CXX=$(or $(TOOLCHAIN_CXX),g++) libminisat.a
minisat22s:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) CXX=$(or $(TOOLCHAIN_CXX),g++) libr
minisat22:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/coreDir -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) CXX=$(or $(TOOLCHAIN_CXX),g++) libr

minisat22d:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/coreDir -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) CXX=$(or $(TOOLCHAIN_CXX),g++) libd

pdtlib:	$(objectdir) $(libdir) $(bindir) $(includedir) $(PDTLIB)

$(EXE): $(EXESRC) $(EXEHDR) $(EXEOBJ) $(PDTLIB) $(LIBS)
	umask 2 ; $(LINKER) $(ALLCFLAGS) $(LDFLAGS) -o $(bindir)/$(EXE) $(INCLUDEDIRS) $(EXEOBJ) $(PDTLIB) $(LIBS) $(OTHERLIBS)

$(APP): $(APPSRC) $(APPHDR) $(APPOBJ) $(PDTLIB) $(LIBS)
	umask 2 ; $(LINKER) $(ALLCFLAGS) $(LDFLAGS) -o $(bindir)/$(APP) $(INCLUDEDIRS) $(APPOBJ) $(PDTLIB) $(LIBS) $(OTHERLIBS)

all : abc aiger lgl minisat $(MINISAT22LIB) cudd pdtlib $(EXE) $(APP)
	  (echo "using $(MINISAT22LIB)")

#--------------------#
# Rules for cleaning #
#--------------------#

clean: pdtrav_clean

distclean: abc_clean aiger_clean lgl_clean cudd_clean minisat_clean minisat22_clean pdtrav_clean
	${FIND} $(PDTOOLS_DIR) -name "#*" -exec rm -f '{}' \; 
	${FIND} $(PDTOOLS_DIR) -name "*~" -exec rm -f '{}' \;

pdtrav_clean:
	rm -rf $(objectdir) $(libdir) $(bindir) $(includedir)
	${RM} $(PDTRAV_DIR)/src/*/#*
	${RM} $(PDTRAV_DIR)/src/*/*~
	${RM} $(FBV_DIR)/*.o
	${RM} $(APP_DIR)/*.o

abc_clean:
	${MAKE} -C ${abc_dir} clean
	${RM} ${abc_dir}/*.a

aiger_clean:
	@if [ -e ${AIGER_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring aiger"; cd ${AIGER_DIR}; ./configure $(DEFINE_PDT32)) \
	fi
	${MAKE} -C ${AIGER_DIR} clean
	${RM} ${AIGER_DIR}/*.a

lgl_clean:
	@if [ -e ${LGL_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring lgl"; cd ${LGL_DIR}; ./configure.sh $(DEFINE_PDT32)) \
	fi
	${MAKE} -C ${LGL_DIR} clean
	${RM} ${LGL_DIR}/*.a

cudd_clean:
	${MAKE} -C ${CUDD_DIR} distclean

minisat_clean:
	${MAKE} -C ${MINISAT_DIR} clean
	${RM} ${MINISAT_DIR}/*.a

minisat22_clean:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/coreDir -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) clean
	${RM} ${MINISAT22_DIR}/*/*.a

