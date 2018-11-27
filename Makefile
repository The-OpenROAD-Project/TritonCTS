# Except as specified in the OpenAccess terms of use of Cadence or Silicon
# Integration Initiative, this material may not be copied, modified,
# re-published, uploaded, executed, or distributed in any way, in any medium,
# in whole or in part, without prior written permission from Cadence.
#
#                Copyright 2002-2005 Cadence Design Systems, Inc.
#                           All Rights Reserved.
#
#  $Author: pdsim $
#  $Revision: #21 $
#  $Date: 2006/11/15 $
# ******************************************************************************
# ******************************************************************************


#include ../macro.defs
SRCDIR = src/ghtree

BINDIR = bin

OBJs = $(BINDIR)/argument.o $(BINDIR)/tree.o $(BINDIR)/design.o $(BINDIR)/main.o $(BINDIR)/mymeasure.o $(BINDIR)/mystring.o

PROG =  $(BINDIR)/genHtree

DEFAULT: $(PROG)

SYSTEM     = x86-64_sles10_4.1
LIBFORMAT  = static_pic

CCPATH   = g++
CPLEXDIR      = /home/tool/ilog/CPLEX_Studio1251/cplex/include
CONCERTDIR    = /home/tool/ilog/CPLEX_Studio1251/concert/include
CXXOPTS  = -m64 -O3 -std=c++11 -fPIC -fno-strict-aliasing -fexceptions -DNDEBUG -DIL_STD -Wno-ctor-dtor-privacy -fopenmp 
CCFLAG = $(CXXOPTS) -I$(CPLEXDIR) -I$(CONCERTDIR) 

SYS_LD_SO = -shared
CXXPIC = -fPIC
SYSLIBS  = -ldl
LIB_SO_EXT = .so
DEBUG = -g 

CPLEXLIBDIR   = $(CPLEXDIR)/../lib/$(SYSTEM)/$(LIBFORMAT)
CONCERTLIBDIR = $(CONCERTDIR)/../lib/$(SYSTEM)/$(LIBFORMAT)

CCLNFLAG = -lm -pthread -fopenmp -L$(CPLEXLIBDIR) -lilocplex -lcplex -L$(CONCERTLIBDIR) -lconcert

 $(BINDIR)/main.o : $(SRCDIR)/main.cpp $(SRCDIR)/argument.h $(SRCDIR)/mystring.h $(SRCDIR)/design.h
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/main.o \
	 -c  $(SRCDIR)/main.cpp 

$(BINDIR)/argument.o :  $(SRCDIR)/argument.cpp  $(SRCDIR)/argument.h  $(SRCDIR)/mystring.h
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/argument.o \
	 -c $(SRCDIR)/argument.cpp 

$(BINDIR)/tree.o : $(SRCDIR)/tree.cpp  $(SRCDIR)/tree.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/tree.o \
	 -c $(SRCDIR)/tree.cpp 

$(BINDIR)/design.o : $(SRCDIR)/design.cpp $(SRCDIR)/design.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/design.o \
	 -c $(SRCDIR)/design.cpp 

$(BINDIR)/mymeasure.o : $(SRCDIR)/mymeasure.cpp $(SRCDIR)/mymeasure.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/mymeasure.o \
	 -c $(SRCDIR)/mymeasure.cpp 

$(BINDIR)/mystring.o : $(SRCDIR)/mystring.cpp $(SRCDIR)/mystring.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o $(BINDIR)/mystring.o \
	 -c $(SRCDIR)/mystring.cpp 

# Link the executable
$(PROG): $(OBJs) 
	$(CCPATH) $(DEBUG) $(CXXOPTS) -o  $(PROG) $(OBJs) \
         $(CCLNFLAG) 

clean: 
	@/bin/rm  -rf $(OBJs) 
	@/bin/rm  -rf $(PROG)
