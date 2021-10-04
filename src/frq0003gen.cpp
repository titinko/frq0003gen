// UTSU vocal synthesis FREQ0003 generator『frq0003gen』 version 0.0.1    2/8/2018
// Branched from MacRes.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>

#include <libpyin/pyin.h>
#include "world.h"
#include "wavread.h"

#include <math.h>

inline DWORD timeGetTime() { return (DWORD)time(NULL) * 1000; }

// 3 inputs:
// 1 Input file
// 2 Output file
// 3 Samples per frq value

#pragma comment(lib, "winmm.lib")

/**
 * Copied from Ameya's world4utau.cpp
 */
double getFrqAvg(double *f0, int num_frames)
{
	int i, j;
	double value = 0, r;
	double p[6], q;
	double freq_avg = 0;
	double base_value = 0;
	for (i = 0; i < num_frames; i++)
	{
		value = f0[i];
		if (value < 1000.0 && value > 55.0)
		{
			r = 1.0;
			// Continuously weight nearby variables more heavily.
			for (j = 0; j <= 5; j++)
			{
				if (i > j) {
					q = f0[i - j - 1] - value;
					p[j] = value / (value + q * q);
				} else {
					p[j] = 1/(1 + value);
				}
				r *= p[j];
			}
			freq_avg += value * r;
			base_value += r;
		}
	}
	if (base_value > 0) freq_avg /= base_value;
	return freq_avg;
}

/**
 * Main method.
 */
int main(int argc, char *argv[])
{
	int i;

	double ms_per_frq;
	double avg_frq;
	double *waveform, *avg_amp;
	int num_samples;
	int num_amp_values;
	int num_frames;

	if(argc < 3) 
	{
		printf("Params: inputfile outputfile samplesPerFrq flags\n");
		return 0;
	}

	int samples_per_frq = 256; // Default value used in UTAU frq files.
	if (argc > 3)
	{
		samples_per_frq = atoi(argv[3]);
	}
	
	/* Parse flags. */
	char *string_buf;
	
	// 'm' flag: method for generating f0.
	// m0 uses libpyin. (default)
	// m1 uses DIO.
	int flag_m = 0;
	if (argc > 4 && (string_buf = strchr(argv[4], 'm')) != 0)
	{
		sscanf(string_buf + 1, "%d", &flag_m);
	}
	
	int sample_rate, bits_per_sample;
	waveform = ReadWaveFile(argv[1], &sample_rate, &bits_per_sample, &num_samples, 
							&num_amp_values, samples_per_frq, &avg_amp);

	if(waveform == NULL)
	{
		fprintf(stderr, "Error: Your input file does not exist.\n");
		return 0;
	}

	printf("File information\n");
	printf("Sampling : %d Hz %d Bit\n", sample_rate, bits_per_sample);
	printf("Length %d [sample]\n", num_samples);
	printf("Length %f [sec]\n", (double)num_samples/(double)sample_rate);

	// Calculate beforehand the number of samples in F0.
	ms_per_frq = samples_per_frq * 1000.0 / sample_rate;
	num_frames = GetNumDIOSamples(sample_rate, num_samples, ms_per_frq);
	if (num_frames != num_amp_values) {
		fprintf(stderr, "Error: num frq values is %d, num amp values is %d.\n", 
				num_frames, num_amp_values);
		free(waveform);
		free(avg_amp);
		return 0;
	}

	// Start to estimate F0 contour (fundamental frequency) using requested method.
	DWORD elapsedTime;
	printf("\nAnalysis\n");
	elapsedTime = timeGetTime();
	double * f0 = (double *) malloc(sizeof(double) * num_frames);

	if (flag_m == 1)
	{
		// DIO method.
		dio(waveform, num_samples, sample_rate, ms_per_frq, f0);
		printf("DIO: %d [msec]\n", timeGetTime() - elapsedTime);
	}
	else 
	{
		// pyin method.
		FP_TYPE* wavform_pyin;
		wavform_pyin = (FP_TYPE *) malloc(sizeof(FP_TYPE) * num_samples);
		for (i = 0; i < num_samples; i++) 
		{
			// Convert wavform to FP_TYPE for now.
			wavform_pyin[i] = (FP_TYPE) waveform[i];
		}
		pyin_config param = pyin_init(samples_per_frq);
		param.fmin = 50.0;
		param.fmax = 800.0;
		param.trange = pyin_trange(param.nq, param.fmin, param.fmax);
		param.nf = samples_per_frq * 5; // Number of samples in an analysis frame.
		param.w = param.nf / 3; // Correlation size in samples.

		int pyin_nfrm = 0;
		FP_TYPE* f0_pyin = pyin_analyze(param, wavform_pyin, num_samples, sample_rate,
			&pyin_nfrm);
		for (i = 0; i < num_frames; i++)
		{
			// Convert from FP_TYPE (float) to double for now.
			f0[i] = (double) ((i >= pyin_nfrm) ? f0_pyin[pyin_nfrm - 1] : f0_pyin[i]);
		}
		free(wavform_pyin);
		free(f0_pyin);
		printf("PYIN: %d [msec]\n", timeGetTime() - elapsedTime);
	}
	avg_frq = getFrqAvg(f0, num_frames);

	// Write output to file.
	char header[40];
	double *output;
	output = (double *)malloc(sizeof(double) * num_frames * 2);
	for (i = 0; i < num_frames; i++) {
		output[i * 2] = f0[i];
		output[(i * 2) + 1] = avg_amp[i];
	}

	header[0] = 'F'; header[1] = 'R'; header[2] = 'E'; header[3] = 'Q';
	header[4] = '0'; header[5] = '0'; header[6] = '0'; header[7] = '3';
	*((int*)(&header[8])) = samples_per_frq;				// Samples per frq value.
	*((double*)(&header[12])) = avg_frq;					// Average frequency.
	for (i = 0; i < 4; i++) {
		*((int*)(&header[20 + (i * 4)])) = 0;				// Empty space.
	}
	*((int*)(&header[36])) = num_frames;					// Number of F0 frames.

	FILE *file = fopen(argv[2], "wb");
	fwrite(header, sizeof(char), 40, file);
	fwrite(output, sizeof(double), num_frames * 2, file);
	fclose(file);
	free(output);

	free(waveform);
	free(avg_amp);
	free(f0);

	fprintf(stderr, "Complete.\n");

	return 0;
}
