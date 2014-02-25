#
# CAIL Makefile
#
# TODO: Should split this into sub-makefiles per component.
#

# Compiler settings.
GPP=g++
ARGS=-Wall -Wextra -Wuninitialized -pedantic
OPT=-O2
#DBG=-g
INCLUDER=-I$(CURDIR)/src/include/

# Valgrind settings.
VALGRIND=valgrind --leak-check=full

BIN=cail
BINDIR=$(CURDIR)/bin
LIBDIR=$(CURDIR)/lib

# CAIL main.
CAILSRC=$(CURDIR)/src/cail/CAIL.cc
CAILINC=$(CURDIR)/src/include/CAIL.h
CAILLIBS=$(LIBDIR)/CAILException.so $(LIBDIR)/CAILPCIInfo.so $(LIBDIR)/CAILCPUInfo.so

# PCI Information getter.
PCISRC=$(CURDIR)/src/cail/CAILPCIInfo.cc
PCIINC=$(CURDIR)/src/include/CAILPCIInfo.h

# CPU Information getter.
CPUSRC=$(CURDIR)/src/cail/CAILCPUInfo.cc
CPUINC=$(CURDIR)/src/include/CAILCPUInfo.h

# CAIL Exception handler.
CAILEXPSRC=$(CURDIR)/src/cail/CAILException.cc
CAILEXPINC=$(CURDIR)/src/include/CAILException.h

# Test command
TEST=$(CURDIR)/bin/./cail -c

test: all
	$(TEST)

all: cail

cail: $(CAILSRC) $(CAILINC) cailexception pciinfo cpuinfo
	$(GPP) $(DBG) $(ARGS) $(OPT) $(INCLUDER) -o $(BINDIR)/$(BIN) $(CAILSRC) $(CAILLIBS)

pciinfo: $(PCISRC) $(PCIINC)
	$(GPP) $(DBG) $(ARGS) $(OPT) $(INCLUDER) -c -o $(LIBDIR)/CAILPCIInfo.so $(PCISRC)

cpuinfo: $(CPUSRC) $(CPUINC)
	$(GPP) $(DBG) $(ARGS) $(OPT) $(INCLUDER) -c -o $(LIBDIR)/CAILCPUInfo.so $(CPUSRC)

cailexception: $(CAILEXPSRC) $(CAILEXPINC)
	$(GPP) $(DBG) $(ARGS) $(OPT) $(INCLUDER) -c -o $(LIBDIR)/CAILException.so $(CAILEXPSRC)
		
valgrind:
	$(VALGRIND) $(TEST) 
	
clean:
	rm -rfv $(BINDIR)/* $(LIBDIR)/*
