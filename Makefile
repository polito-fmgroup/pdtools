# Top-level Makefile for pdtools
#
# Builds the vendored libraries (ABC, CUDD, AIGER, Lingeling, MiniSat) and
# then pdtrav and pdtapp. Each library is built by running its own Makefile
# through pdtrav/Makefile; this file decides when that needs to happen. The
# make target for a library is its archive (e.g. abc/libabc.a), so a library
# is rebuilt only when its sources are newer than the archive.
#
# Run `make help` for the available targets. Compilers and flags come from
# config.mk.

# Grouped targets (`&:`, used for the CUDD libraries below) need GNU Make 4.3
# or later. Stop here with a clear message instead of failing on the syntax.
ifeq ($(filter grouped-target,$(.FEATURES)),)
$(error GNU Make 4.3 or newer is required (this is $(MAKE_VERSION)))
endif

include config.mk

.PHONY: all check-deps vendor-libs pdtrav-bin clean distclean help info

.DEFAULT_GOAL := all

# Verify the required programs and the readline header before building, so
# a missing package is reported up front instead of partway through the build.
check-deps:
	@status=0; \
	for tool in $(REQUIRED_TOOLS); do \
		if ! command -v $$tool >/dev/null 2>&1; then \
			echo "MISSING: '$$tool' not found on PATH"; \
			status=1; \
		fi; \
	done; \
	if ! printf '#include <readline/readline.h>\nint main(void){return 0;}\n' | \
		$(TOOLCHAIN_CC) -x c -o /dev/null - >/dev/null 2>&1; then \
		echo "MISSING: readline development headers (e.g. 'libreadline-dev' on Debian/Ubuntu)"; \
		status=1; \
	fi; \
	if [ $$status -ne 0 ]; then \
		echo ""; \
		echo "Preflight check failed: install the missing item(s) above before building."; \
		echo "On Debian/Ubuntu:"; \
		echo "    apt-get install -y build-essential flex bison libreadline-dev"; \
		exit 1; \
	fi; \
	echo "check-deps: all required tools and headers found."

# Vendored libraries and the archives they produce.
ABC_DIR     = abc
CUDD_DIR    = cudd-2.5.0
AIGER_DIR   = aiger-1.9.3
LGL_DIR     = lingeling-bcj
MINISAT_DIR = minisat-p_v1.14
MINISAT22_DIR = minisat-2.2.0-drup

ABC_LIB       = $(ABC_DIR)/libabc.a
AIGER_LIB     = $(AIGER_DIR)/libaiger.a
LGL_LIB       = $(LGL_DIR)/liblgl.a
MINISAT_LIB   = $(MINISAT_DIR)/libminisat.a
MINISAT22_LIB = $(MINISAT22_DIR)/simp/lib.a
CUDD_LIBS     = $(CUDD_DIR)/dddmp/libdddmp.a $(CUDD_DIR)/cudd/libcudd.a \
                $(CUDD_DIR)/mtr/libmtr.a $(CUDD_DIR)/st/libst.a \
                $(CUDD_DIR)/util/libutil.a $(CUDD_DIR)/epd/libepd.a

# Source files of each library, used as prerequisites of its archive. Without
# them an existing archive would always count as up to date and source changes
# would never trigger a rebuild. The lists are computed once, when the
# Makefile is parsed.
#
# Only sources are tracked. Changing the compiler, BUILD_MODE or other flags
# does not rebuild an existing archive: run `make distclean` first.
ABC_SRCS       = $(shell find $(ABC_DIR)/src -name '*.c' -o -name '*.h' 2>/dev/null)
CUDD_SRCS      = $(shell find $(CUDD_DIR) -name '*.c' -o -name '*.h' 2>/dev/null)
AIGER_SRCS     = $(shell find $(AIGER_DIR) -maxdepth 1 \( -name '*.c' -o -name '*.h' \) 2>/dev/null)
LGL_SRCS       = $(shell find $(LGL_DIR) -maxdepth 1 \( -name '*.c' -o -name '*.h' \) 2>/dev/null)
MINISAT_SRCS   = $(shell find $(MINISAT_DIR) -name '*.C' -o -name '*.h' 2>/dev/null)
MINISAT22_SRCS = $(shell find $(MINISAT22_DIR) -name '*.cc' -o -name '*.h' 2>/dev/null)

$(ABC_LIB): $(ABC_SRCS)
	$(MAKE) -C pdtrav abc

$(AIGER_LIB): $(AIGER_SRCS)
	$(MAKE) -C pdtrav aiger

$(LGL_LIB): $(LGL_SRCS)
	$(MAKE) -C pdtrav lgl

$(MINISAT_LIB): $(MINISAT_SRCS)
	$(MAKE) -C pdtrav minisat

$(MINISAT22_LIB): $(MINISAT22_SRCS)
	$(MAKE) -C pdtrav $(MINISAT22LIB)

# CUDD's sub-make builds all six archives, so they are declared as a grouped
# target (`&:`, GNU Make 4.3 or later): one run of the recipe updates all of
# them. The sub-make only rebuilds the archives affected by a change, so the
# touch afterwards keeps every archive newer than the sources. Otherwise this
# rule would run again on every build.
$(CUDD_LIBS) &: $(CUDD_SRCS)
	$(MAKE) -C pdtrav cudd
	@touch $(CUDD_LIBS)

vendor-libs: $(ABC_LIB) $(AIGER_LIB) $(LGL_LIB) $(MINISAT_LIB) $(MINISAT22_LIB) $(CUDD_LIBS)

# Build the pdtrav library and link the pdtrav and pdtapp executables.
# The `pdtrav` and `pdtapp` targets in pdtrav/Makefile do not name the files
# they create (bin/pdtrav, bin/pdtapp), so make cannot see that they are up
# to date and the link step runs on every invocation.
pdtrav-bin: vendor-libs
	$(MAKE) -C pdtrav pdtlib pdtrav pdtapp

all: check-deps pdtrav-bin
	@echo ""
	@echo "Build complete:"
	@echo "  pdtrav/bin/pdtrav"
	@echo "  pdtrav/bin/pdtapp"

# Remove pdtrav's objects, library and executables. The vendored libraries
# are kept, so the next build does not recompile them.
clean:
	$(MAKE) -C pdtrav pdtrav_clean

# Remove all build output, including the vendored libraries.
distclean:
	$(MAKE) -C pdtrav distclean

info:
	@echo "BUILD_MODE     = $(BUILD_MODE)"
	@echo "TOOLCHAIN_CC   = $(TOOLCHAIN_CC)"
	@echo "TOOLCHAIN_CXX  = $(TOOLCHAIN_CXX)"
	@echo "CXX_STD        = $(CXX_STD)"
	@echo "ARCH_FLAGS     = $(ARCH_FLAGS)"
	@if [ -f config.local.mk ]; then \
		echo ""; \
		echo "  (from config.local.mk — generated by ./configure)"; \
		echo "  OS             = $(CONFIGURE_OS)"; \
		echo "  readline dev   = $(CONFIGURE_READLINE_PKG)"; \
		echo "  readline rt    = $(CONFIGURE_READLINE_RUNTIME_PKG)"; \
	else \
		echo ""; \
		echo "  (no config.local.mk — run ./configure for platform-specific setup)"; \
	fi

help:
	@echo "pdtools build — quick start:"
	@echo "  ./configure          - probe system, select compiler, write config.local.mk"
	@echo "  ./configure --help   - show all configure options"
	@echo ""
	@echo "make targets:"
	@echo "  make                 - build pdtrav + pdtapp (default mode)"
	@echo "  make check-deps      - verify required tools/headers are installed"
	@echo "  make vendor-libs     - build only the third-party libraries"
	@echo "  make clean           - remove pdtrav build output (keeps vendor libs)"
	@echo "  make distclean       - remove all build output including vendor libs"
	@echo "  make info            - show resolved toolchain/flags"
	@echo ""
	@echo "  make BUILD_MODE=debug      - debug build (-g, no optimization)"
	@echo "  make BUILD_MODE=sanitize   - AddressSanitizer build"
	@echo "  make ARCH_FLAGS=\"-mtune=native -malign-double\"  - host-tuned, non-portable binaries"
