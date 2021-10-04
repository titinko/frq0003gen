export FP_TYPE ?= float

LIB_PREFIX=../..
OBJS = dio.o matlabfunctions.o frq0003gen.o fft.o wavread.o
CXX = g++
CFLAGS = -O -DFP_TYPE=$(FP_TYPE)
INCLUDES = -I. -I$(LIB_PREFIX)/include
LIBS = $(LIB_PREFIX)/lib/libpyin.a $(LIB_PREFIX)/lib/libgvps.a

frq0003gen : $(OBJS) $(LIBS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm frq0003gen $(OBJS)
