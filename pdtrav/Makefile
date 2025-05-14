#------------------------------------------#
# Makefile for the PdTRAV distribution kit #*
# Version 3.5.0                            #
#------------------------------------------#

# Commands Available:
# make
#    all (the default)
#    it makes all: compiles the external libraries, creates the pdtrav library, makes pdtrav
# make clean
#    it cleans pdtrav
# make distclean
#    it cleans pdtrav and all other packages

#------------#
# For safety #
#------------#

SHELL = /bin/sh

.SUFFIXES:
#----------------------------------------#
# Configuration Section                  #
# uncomment the desired options/sections #
#----------------------------------------#

.PHONY : default static 

default: LDFLAGS = 
static : LDFLAGS = -static
sanitize : LDFLAGS = -fsanitize=address -static-libasan

default: all
static : all
sanitize : all

ifdef (PDT32)
DEFINE_PDT32 = PDT32=yes
else
DEFINE_PDT32 =
endif

ifdef DEBUG
MINISAT22LIB = minisat22d
else
MINISAT22LIB = minisat22
endif

# Memory libraries
#MNEMLIB	=
# MNEMLIB	= /usr/lib/libdmalloc.a

# ReadLine Library
#READLINEFLAGS =
#READLINEFLAGS = -DHAVE_LIBREADLINE -DHAVE_READLINE_READLINE_H -DHAVE_READLINE_HISTORY_H
#PDTRAVLIBS += -lreadline
#READLINEFLAGS = -DHAVE_LIBREADLINE -DHAVE_READLINE_READLINE_H
#PDTRAVLIBS += -lreadline

# Level of Self-Checking and Verbosity of the CUDD Package

#DDEBUG =
#DDDEBUG = -DDD_DEBUG -DDD_VERBOSE -DDD_STATS -DDD_CACHE_PROFILE -DDD_UNIQUE_PROFILE -DDD_COUNT -DDDDMP_DEBUG
#DDDEBUG = -DDDDMP_DEBUG

# Specify compilation flags (ICFLAGS are for CUDD only!)
CC =		g++
CUDD_CC =	gcc
#CUDD_CC =	g++
#GCC =		gcc

#LGLDBGCFLAGS =	-Wall -g -DNLGLOG -DNDEBUG -DNCHKSOL -DNLGLDRUPLIG -DNLGLYALSAT -DNLGLFILES -DNLGLDEMA 
LGLDBGCFLAGS =	-Wall -g -DNLGLOG -DNCHKSOL -DNLGLDRUPLIG -DNLGLYALSAT -DNLGLFILES -DNLGLDEMA 


ifdef PDT32
# StQ: 32-bit (2 rows) OK x cudd-2.4.1 OK ANCHE PER cudd-2.4.2 ma compilato con gcc e non con g++
CUDD_ICFLAGS = 	-O6 -m32 -Wall -pthread
# TOLTE O MESSE SEMBRA NON FARE DIFFERENZA -Wall -fPIC  -pthread
CUDD_XCFLAGS =	-mtune=native -fPIC -DHAVE_IEEE_754 -DBSD
# TOLTE O MESSE SEMBRA NON FACCIA DIFFERENZA --> -DSTDC_HEADERS -DHAVE_STRSTR
#XC_FLAGS = 	-pthread -m32 -mtune=native -DHAVE_IEEE_754 -DBSD
XC_FLAGS =
# StQ: 32-bit
#CFLAGS =	-Wall -g3 -m32 -pthread -static
CFLAGS =	-DLIN -g -m32 -pthread
else
# StQ: 64-bit (2 rows)
CUDD_ICFLAGS = 	-O6 -Wall -fPIE -pthread -std=c++14
#CUDD_ICFLAGS = 	-g -Wall -fPIC -pthread
CUDD_XCFLAGS =	-ffloat-store -mtune=native -malign-double -DHAVE_IEEE_754 -DBSD -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8 -DSIZEOF_INT=4
#ICFLAGS = 	-Wall -O6 -fPIC -pthread -mtune=native -malign-double -DSIZEOF_VOID_P=8 -DSIZEOF_LONG=8
XC_FLAGS =	-DHAVE_IEEE_754=1 -DBSD=1 -DSTDC_HEADERS=1 -DHAVE_STRSTR=1
ifdef PDTOPT
CFLAGS =	-DLIN64 -O6 -Wall -fPIE -pthread
else
CFLAGS =	-DLIN64 -g -Wall -fPIE -pthread 
endif
endif
LINKER =	g++
#LDFLAGS =	-pthread -static --enable-static --disable-shared
#LDFLAGS =	-pthread -static -ltcmalloc
#LDFLAGS =	-pthread -static
#LDFLAGS =	-pthread
#LDFLAGS =	-static

CFLAGS += 	$(CUDD_XCFLAGS) -std=c++14
OTHERLIBS =	-lreadline -lncurses -ltermcap -ldl -lrt -lm
#OTHERLIBS =	-lreadline -ldl -lrt -lm
YACC =		bison -y
LEX =		flex
NAWK =		gawk
AR =		ar
RANLIB =	ranlib
RM =		rm -f
FIND =		find

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
#minisat22lib = $(MINISAT22_DIR)/coreDir/lib.a

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
# Form the the list of directories to search for header files. #
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
	#ln -s $(ABC_DIR)/src/misc/st/*.h $(includedir)
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
# x cudd-2.4.2
#	${MAKE} CC=$(GCC) XCFLAGS="$(XC_FLAGS)" -C ${CUDD_DIR} cuddlibs

abc:
	${MAKE} -C ${ABC_DIR} $(DEFINE_PDT32) libabc.a

aiger:
	@if [ -e ${AIGER_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring aiger"; cd ${AIGER_DIR}; ./configure $(DEFINE_PDT32)) \
	fi
	${MAKE} -C ${AIGER_DIR} libaiger.a

lgl:
	@if [ -e ${LGL_DIR}/makefile ] ; then \
		(echo -n "") ; \
	else \
	        (echo "Configuring lgl"; cd ${LGL_DIR}; ./configure.sh $(DEFINE_PDT32)) \
	fi
	${MAKE} -C ${LGL_DIR} liblgl.a
#	${MAKE} CFLAGS="$(LGLDBGCFLAGS)" -C ${LGL_DIR} liblgl.a

minisat:
	${MAKE} -C ${MINISAT_DIR} $(DEFINE_PDT32) libminisat.a
minisat22s:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) libr
minisat22:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/coreDir -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) libr

minisat22d:
	${MAKE} MROOT=${MINISAT22_DIR} -C ${MINISAT22_DIR}/coreDir -C ${MINISAT22_DIR}/simp $(DEFINE_PDT32) libd

pdtlib:	$(objectdir) $(libdir) $(bindir) $(includedir) $(PDTLIB)

$(EXE): $(EXESRC) $(EXEHDR) $(EXEOBJ) $(PDTLIB) $(LIBS)
	umask 2 ; $(LINKER) $(ALLCFLAGS) $(LDFLAGS) -o $(bindir)/$(EXE) $(INCLUDEDIRS) $(EXEOBJ) $(PDTLIB) $(LIBS) $(OTHERLIBS)

$(APP): $(APPSRC) $(APPHDR) $(APPOBJ) $(PDTLIB) $(LIBS)
	umask 2 ; $(LINKER) $(ALLCFLAGS) $(LDFLAGS) -o $(bindir)/$(APP) $(INCLUDEDIRS) $(APPOBJ) $(PDTLIB) $(LIBS) $(OTHERLIBS)

#$(EXE): $(EXESRC) $(EXEHDR) $(EXEOBJ) $(PDTLIB) $(LIBS)
#	umask 2 ; $(LINKER) $(ALLCFLAGS) -o $(bindir)/$(EXE) $(INCLUDEDIRS) $(EXESRC) $(PDTLIB) $(LIBS) $(OTHERLIBS)

all : abc aiger lgl minisat $(MINISAT22LIB) cudd pdtlib $(EXE) $(APP)
	  (echo "using $(MINISAT22LIB)")

#all : abc aiger minisat minisat22s cudd pdtlib $(EXE)

#--------------------#
# Rules for cleaning #
#--------------------#

clean: pdtrav_clean

#distclean: aiger_clean cudd_clean minisat_clean pdtrav_clean
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

