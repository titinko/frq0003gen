OBJS = dio.o matlabfunctions.o frq0003gen.o fft.o wavread.o
CXX = g++
CFLAGS = -O -msse3
INCLUDES = -I.
LIBS = 

frq0003gen : $(OBJS) fft.h mmsystem.h windows.h wavread.h world.h
	$(CXX) -o $@ $(OBJS) $(LIBS) $(CFLAGS)

.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm frq0003gen $(OBJS)
