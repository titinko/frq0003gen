// UTSU vocal synthesis FREQ0003 generator『frq0003gen』 version 0.0.1    2/8/2018
// Branched from MacRes.
// Uses the FFTW library.

#include <fft.h>

#include <stdlib.h>
#include <windows.h>
#include <math.h>

#define PI 3.1415926535897932384

// For Windows
#pragma warning( disable : 4996 )

#pragma comment(lib, "libfftw3-3.lib")
#pragma comment(lib, "libfftw3f-3.lib")
#pragma comment(lib, "libfftw3l-3.lib")

#define MAX_FFT_LENGTH 2048
#define FLOOR_F0 90.0//71.0    tn_fnds v0.0.4 低い方向へのF0誤検出防止。UTAUの原音はそんなに低くないだろうと楽観
#define DEFAULT_F0 500.0//150.0   tn_fnds v0.0.3 大きくしてみたら無声子音がきれいになった
#define LOW_LIMIT 65.0//EFB-GT

// 71は，fs: 44100においてFFT長を2048にできる下限．
// 70 Hzにすると4096点必要になる．
// DEFAULT_F0は，0.0.4での新機能．調整の余地はあるが，暫定的に決定する．

// F0 Estimator DIO : Distributed Inline-filter Operation
void dio(double *x, int xlen, int fs, double frame_period, double *f0);
int GetNumDIOSamples(int sample_rate, int num_samples, double frame_period);

//------------------------------------------------------------------------------------
// Migrated from Matlab.
double std2(double *x, int xLen);
void inv(double **r, int n, double **invr);
float randn(void);
void histc(double *x, int xLen, double *y, int yLen, int *index);
void interp1(double *t, double *y, int iLen, double *t1, int oLen, double *y1);
long decimateForF0(double *x, int xLen, double *y, int r);
void filterForDecimate(double *x, int xLen, double *y, int r);
int myround(double x);
void diff(double *x, int xLength, double *ans);
void interp1Q(double x, double shift, double *y, int xLength, double *xi, int xiLength, double *ans);
