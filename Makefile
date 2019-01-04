SOURCEDIR = .
OBJECTDIR := ./obj
REAL_SOURCEDIR := $(shell cd $(SOURCEDIR); pwd)
REAL_OBJECTDIR := $(shell mkdir -p $(OBJECTDIR); cd obj; pwd)

include ./common/Toolchain.mk

#CXXFLAGS += -fno-rtti 
CXXFLAGS += $(CPUOPTION)
CXXFLAGS += -fexceptions -fno-permissive

ifeq ($(OPT_DEBUG),true)
CXXFLAGS	+= -g -DDEBUG -D_DEBUG -Wall
else
CXXFLAGS	+= -O2
endif

WARN := -Wall -Wno-switch-enum -Wno-switch -Wno-multichar -Wno-shift-overflow -Wno-unused-variable -Wno-unused-but-set-variable
PREPROC := -D__int64="long long"

EXECUTABLE := dumpmp4av
LIBRARY := libmp4rawdatafix

SOURCES := \
	$(SOURCEDIR)/AMRingBuffer.cpp \
	$(SOURCEDIR)/Bitstream.cpp \
	$(SOURCEDIR)/StdAfx.cpp \
	$(SOURCEDIR)/DumpAV.cpp \
	$(SOURCEDIR)/DumpMDAT.cpp \
	$(SOURCEDIR)/aacsyntax.cpp \
	$(SOURCEDIR)/huffman.cpp
	
TESTSOURCE := \
	$(SOURCEDIR)/Test.cpp
	
OBJECTS = $(patsubst $(SOURCEDIR)/%.c, $(OBJECTDIR)/%.o, $(patsubst $(SOURCEDIR)/%.cpp, $(OBJECTDIR)/%.o, $(SOURCES)))
TESTOBJECTS = $(patsubst $(SOURCEDIR)/%.c, $(OBJECTDIR)/%.o, $(patsubst $(SOURCEDIR)/%.cpp, $(OBJECTDIR)/%.o, $(TESTSOURCE)))

INCDIRS += \
	-I. \
	-I./LibPlatform
	
CPPFLAGS = $(DBG) $(OPT) $(WARN) $(COMMON_PREPROC) $(PREPROC) $(INCDIRS)

$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.cpp
	@if [ ! -d $(OBJECTDIR)/ ]; then mkdir $(OBJECTDIR)/ ; fi
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $($@_XFLAGS) -fPIC $< -o $@
$(OBJECTDIR)/%.o: $(SOURCEDIR)/%.c
	@if [ ! -d $(OBJECTDIR)/ ]; then mkdir $(OBJECTDIR)/ ; fi
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $($@_XFLAGS) $< -o $@

all: $(LIBRARY).so $(EXECUTABLE) $(EXTRATARGETS)

$(LIBRARY).so: $(OBJECTS)
	@if [ ! -d $(BINDIR)/ ]; then mkdir -p $(BINDIR)/ ; fi
	$(CXX) -shared -L$(BINDIR) -o $@ $^ -Wl,-Bsymbolic
ifneq ($(OPT_DEBUG),true)
	$(STRIP) $(LIBRARY).so
endif
	cp -f $@ $(BINDIR)/
	
$(EXECUTABLE): $(TESTOBJECTS)
	@if [ ! -d $(BINDIR)/ ]; then mkdir -p $(BINDIR)/ ; fi
	$(CXX) -o $@ $^ -Wl,-Bsymbolic,--allow-shlib-undefined,-rpath,. -L$(BINDIR) -L./ -lmp4rawdatafix -ldl
ifneq ($(OPT_DEBUG),true)
	$(STRIP) $(EXECUTABLE)
endif
	cp -f $@ $(BINDIR)/	

clean:
	@echo $(SOURCEDIR)
	@echo $(OBJECTDIR)
	rm -rf $(LIBRARY).so $(EXECUTABLE) $(OBJECTS) $(TESTOBJECTS) $(OBJECTDIR)
