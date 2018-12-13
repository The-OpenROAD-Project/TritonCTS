////////////////////////////////////////////////////////////////////////////////////
// Authors: Kwangsoo Han and Jiajia Li
//          (Ph.D. advisor: Andrew B. Kahng),
//          Many subsequent changes for open-sourcing were made by Mateus Fogaça
//          (Ph.D. advisor: Ricardo Reis)
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////

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
