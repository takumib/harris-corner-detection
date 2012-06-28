#
# CUDA Makefile, based on http://www.eecs.berkeley.edu/~mjmurphy/makefile
#

# Location of the gimp plugin directory
INSTALL_PATH    = $(HOME)/.gimp-2.6/plug-ins

# handle spaces in SDK filename
space = $(empty) $(empty)
# $(call space_to_question,file_name)
space_to_question = $(subst $(space),?,$1)
# $(call wildcard_spaces,file_name)
wildcard_spaces = $(wildcard $(call space_to_question,$(CUDA_SDK_PATH)/C))

# Fill in the name of the output binary here
TARGET    := canny_filter

# List of sources, with .c, and .cpp extensions
SOURCES := \
    gimp_gui.cpp \
    gimp_main.cpp \
    multi_res_cpu.cpp \


# Other things that need to be built, e.g. .cubin files
EXTRADEPS := \
    Makefile \
    multi_res_host.hpp \
    defines_cpu.hpp \
    gimp_gui.hpp \
    gimp_main.hpp \
    multi_res_cpu.hpp


# Flags common to all compilers. You can set these on the comamnd line, e.g:
# $ make opt="" dbg="-g" warn="-Wno-deptrcated-declarations -Wall -Werror"

OPT  ?= -O3
DBG  ?= 
WARN ?= -Wall

# flags required to link and compile against gimp
INCLUDE_GIMP_pt = $(shell pkg-config --cflags gimpui-2.0)
INCLUDE_GIMP	= $(subst -pthread,,$(INCLUDE_GIMP_pt))
LINK_GIMP       = $(shell pkg-config --libs gimpui-2.0)

#----- C compilation options ------
GCC         := /usr/bin/gcc

CFLAGS     += $(BIT) $(OPT) $(DBG) $(WARN)
CLIB_PATHS :=
CLIBRARIES := 


#----- C++ compilation options ------
GPP         := /usr/bin/g++
CCFLAGS     += $(BIT) $(OPT) $(DBG) $(WARN)
CCLIB_PATHS := $(LINK_GIMP) -L$(SDK_PATH)/common/lib
CCINC_PATHS := $(INCLUDE_GIMP) -I$(SDK_PATH)/common/inc
CCLIBRARIES := $(LCUTIL)

LIB_PATHS   := $(CULIB_PATHS) $(CCLIB_PATHS) $(CLIB_PATHS)
LIBRARIES   := $(CULIBRARIES) $(CCLIBRARIES) $(CLIBRARIES)


#----- Generate source file and object file lists
# This code separates the source files by filename extension into C, C++,
# and Cuda files.

CSOURCES  := $(filter %.c ,$(SOURCES))
CCSOURCES := $(filter %.cpp,$(SOURCES))

# This code generates a list of object files by replacing filename extensions

OBJECTS := $(patsubst %.c,%.o ,$(CSOURCES))  \
           $(patsubst %.cpp,%.o,$(CCSOURCES))

#----- Build rules ------

$(TARGET): $(EXTRADEPS) 


$(TARGET): $(OBJECTS)
	$(GPP) -fPIC $(BIT) -o $@ $(OBJECTS) $(LIB_PATHS) $(LIBRARIES)

%.o: %.cpp
	$(GPP) -c $^ $(CCFLAGS) $(CCINC_PATHS) -o $@

%.o: %.c
	$(GCC) -c $^ $(CFLAGS) $(CINC_PATHS) -o $@

clean:
	rm -f *.o $(TARGET) *.linkinfo

install: $(TARGET)
	cp $(TARGET) $(INSTALL_PATH)/

