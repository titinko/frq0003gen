# FRQ0003GEN - A cross-platform .frq file generator

(Fork of https://github.com/titinko/Macres)

frq0003gen takes as input a 16-bit wav file and outputs a FREQ0003 (.frq) file that can be
read by UTSU, UTAU, OpenUTAU, or similar programs.

This is an executable designed to be called by https://github.com/titinko/utsu (UTSU).
It can be compiled for Windows, Mac, or Linux using the Makefile.

Sample usage:
````
./frq0003gen de.wav de_wav.frq 256
````

The current version of frq0003gen uses [libpyin](https://github.com/Sleepwalking/libpyin) to estimate frequency, but if you want to use the legacy method (DIO) you can re-enable it with a flag:
````
./frq0003gen de.wav de_wav.frq 256 m1
````

## Building from source

This program uses two external libraries: [libpyin](https://github.com/Sleepwalking/libpyin) and [libgvps](https://github.com/Sleepwalking/libpyin).

You will need to git clone both repositories, install their libraries using make install, and update the LIB_PREFIX field in frq0003gen's Makefile to match the directory where the .a and .h files were generated. You will also need to manually edit pyin.h to wrap its function definitions with "extern C" as described in [this bug](https://github.com/Sleepwalking/libpyin/issues/10).
