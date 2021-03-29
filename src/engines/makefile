CLEAN_FILES = # deliberately empty, so we can append below.
CXX=g++
PLATFORM_LDFLAGS= -lpthread -lrt -pmem -lpmemobj
PLATFORM_CXXFLAGS= -std=c++11
PROFILING_FLAGS=
OPT=-msse4 -mavx -mavx2
LDFLAGS += -Wl,-rpath=$(RPATH)

# DEBUG_LEVEL can have two values:
# * DEBUG_LEVEL=2; this is the ultimate debug mode. It will compile benchmark
# without any optimizations. To compile with level 2, issue `make dbg`
# * DEBUG_LEVEL=0; this is the debug level we use for release. If you're
# running benchmark in production you most definitely want to compile benchmark
# with debug level 0. To compile with level 0, run `make`,

# Set the default DEBUG_LEVEL to 0
DEBUG_LEVEL?=0

ifeq ($(MAKECMDGOALS),dbg)
  DEBUG_LEVEL=2
endif

# compile with -O2 if debug level is not 2
ifneq ($(DEBUG_LEVEL), 2)
OPT += -O3 -fno-omit-frame-pointer
# if we're compiling for release, compile without debug code (-DNDEBUG) and
# don't treat warnings as errors
OPT += -DNDEBUG
DISABLE_WARNING_AS_ERROR=1
# Skip for archs that don't support -momit-leaf-frame-pointer
ifeq (,$(shell $(CXX) -fsyntax-only -momit-leaf-frame-pointer -xc /dev/null 2>&1))
OPT += -momit-leaf-frame-pointer
endif
else
$(warning Warning: Compiling in debug mode. Don't use the resulting binary in production)
OPT += $(PROFILING_FLAGS)
DEBUG_SUFFIX = "_debug"
DISABLE_WARNING_AS_ERROR=1
endif

# ----------------------------------------------
SRC_PATH = $(CURDIR)

# ----------------Dependences-------------------

INCLUDE_PATH = -I./ -I../

# ---------------End Dependences----------------

LIB_SOURCES := $(wildcard $(SRC_PATH)/*.cpp)

#-----------------------------------------------

AM_DEFAULT_VERBOSITY = 0

AM_V_GEN = $(am__v_GEN_$(V))
am__v_GEN_ = $(am__v_GEN_$(AM_DEFAULT_VERBOSITY))
am__v_GEN_0 = @echo "  GEN     " $(notdir $@);
am__v_GEN_1 =
AM_V_at = $(am__v_at_$(V))
am__v_at_ = $(am__v_at_$(AM_DEFAULT_VERBOSITY))
am__v_at_0 = @
am__v_at_1 =

AM_V_CC = $(am__v_CC_$(V))
am__v_CC_ = $(am__v_CC_$(AM_DEFAULT_VERBOSITY))
am__v_CC_0 = @echo "  CC      " $(notdir $@);
am__v_CC_1 =
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
AM_V_CCLD = $(am__v_CCLD_$(V))
am__v_CCLD_ = $(am__v_CCLD_$(AM_DEFAULT_VERBOSITY))
am__v_CCLD_0 = @echo "  CCLD    " $(notdir $@);
am__v_CCLD_1 =

AM_LINK = $(AM_V_CCLD)$(CXX) $^ $(EXEC_LDFLAGS) -o $@ $(LDFLAGS)

CXXFLAGS += -g

# This (the first rule) must depend on "all".
default: all

WARNING_FLAGS = -W -Wextra -Wall -Wsign-compare \
  							-Wno-unused-parameter -Woverloaded-virtual \
								-Wnon-virtual-dtor -Wno-missing-field-initializers

ifndef DISABLE_WARNING_AS_ERROR
  WARNING_FLAGS += -Werror
endif

CXXFLAGS += $(WARNING_FLAGS) $(INCLUDE_PATH) $(PLATFORM_CXXFLAGS) $(OPT)

LDFLAGS += $(PLATFORM_LDFLAGS)

LIBOBJECTS = $(LIB_SOURCES:.cpp=.o)
# if user didn't config LIBNAME, set the default
ifeq ($(LIBNAME),)
# we should only run benchmark in production with DEBUG_LEVEL 0
LIBNAME=libengine$(DEBUG_SUFFIX)
endif

ifeq ($(LIBOUTPUT),)
LIBOUTPUT=$(CURDIR)/lib
endif

ifeq ($(EXEC_DIR),)
EXEC_DIR=$(CURDIR)
endif

dummy := $(shell mkdir -p $(LIBOUTPUT))
LIBRARY = $(LIBOUTPUT)/${LIBNAME}.a
INCLUDE_PATH += -I$(EXEC_DIR)

.PHONY: clean dbg all

%.o: %.cpp
	  $(AM_V_CC)$(CXX) $(CXXFLAGS) -c $< -o $@

all: $(LIBRARY)

dbg: $(LIBRARY)

$(LIBRARY): $(LIBOBJECTS)
	$(AM_V_at)rm -f $@
	$(AM_V_at)$(AR) $(ARFLAGS) $@ $(LIBOBJECTS)
	
clean:
	rm -f $(LIBRARY)
	rm -rf $(CLEAN_FILES)
	rm -rf $(LIBOUTPUT)
	find $(SRC_PATH) -maxdepth 1 -name "*.[oda]*" -exec rm -f {} \;
	find $(SRC_PATH) -maxdepth 1 -type f -regex ".*\.\(\(gcda\)\|\(gcno\)\)" -exec rm {} \;