// UTSU vocal synthesis FREQ0003 generator『frq0003gen』 version 0.0.1    2/8/2018
// Branched from MacRes.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <windows.h>

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
	double *waveform, *f0, *avg_amp;
	int num_samples;
	int num_amp_values;
	int num_frames;

	if(argc < 4) 
	{
		printf("Params: inputfile outputfile samplesPerFrq\n");
		return 0;
	}

	FILE *file;

	int sample_rate, bits_per_sample;
	waveform = ReadWaveFile(argv[1], &sample_rate, &bits_per_sample, &num_samples, 
							&num_amp_values, atoi(argv[3]), &avg_amp);

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
	ms_per_frq = atoi(argv[3]) * 1000.0 / sample_rate;
	num_frames = GetNumDIOSamples(sample_rate, num_samples, ms_per_frq);
	if (num_frames != num_amp_values) {
		fprintf(stderr, "Error: num frq values is %d, num amp values is %d.\n", 
				num_frames, num_amp_values);
		free(waveform);
		free(avg_amp);
		return 0;
	}
	f0 = (double *)malloc(sizeof(double)*num_frames);

	// Start to estimate F0 contour (fundamental frequency) using DIO.
	DWORD elapsedTime;
	printf("\nAnalysis\n");
	elapsedTime = timeGetTime();

	dio(waveform, num_samples, sample_rate, ms_per_frq, f0);
	avg_frq = getFrqAvg(f0, num_frames);
	printf("DIO: %d [msec]\n", timeGetTime() - elapsedTime);

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
	*((int*)(&header[8])) = 256;							// Samples per frq value.
	*((double*)(&header[12])) = avg_frq;					// Average frequency.
	for (i = 0; i < 4; i++) {
		*((int*)(&header[20 + (i * 4)])) = 0;				// Empty space.
	}
	*((int*)(&header[36])) = num_frames;					// Number of F0 frames.

	file = fopen(argv[2], "wb");
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
