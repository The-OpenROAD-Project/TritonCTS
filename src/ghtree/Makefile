
OBJs = argument.o tree.o design.o main.o mymeasure.o mystring.o

PROG = genHtree

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

main.o : main.cpp argument.h mystring.h design.h
	$(CCPATH) $(CCFLAG) $(DEBUG) -o main.o \
	 -c main.cpp 

argument.o : argument.cpp argument.h mystring.h
	$(CCPATH) $(CCFLAG) $(DEBUG) -o argument.o \
	 -c argument.cpp 

tree.o : tree.cpp tree.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o tree.o \
	 -c tree.cpp 

design.o : design.cpp design.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o design.o \
	 -c design.cpp 

mymeasure.o : mymeasure.cpp mymeasure.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o mymeasure.o \
	 -c mymeasure.cpp 

mystring.o : mystring.cpp mystring.h 
	$(CCPATH) $(CCFLAG) $(DEBUG) -o mystring.o \
	 -c mystring.cpp 

# Link the executable
$(PROG): $(OBJs) 
	$(CCPATH) $(DEBUG) $(CXXOPTS) -o $(PROG) $(OBJs) \
         $(CCLNFLAG) 

clean: 
	@/bin/rm  -rf $(OBJs) 
	@/bin/rm  -rf $(PROG)
