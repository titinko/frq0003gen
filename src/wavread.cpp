// UTSU vocal synthesis FREQ0003 generator『frq0003gen』 version 0.0.1    2/8/2018
// Branched from MacRes.
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "wavread.h"

#pragma warning(disable:4996)

/**
 * Reads a signal from a WAV file.
 * param filename: file name to read from
 * param sample_rate: place to put sample frequency (Hz) of signal
 * param bits_per_sample: place to put number of bits used for a sample
 * param num_samples: place to put total number of samples read
 */
double * ReadWaveFile(
	char *filename,
	int *sample_rate,
	int *bits_per_sample,
	int *num_samples,
	int *num_amp_values,
	int samples_per_frq,
	double **avg_amp)
{
	FILE *file;
	char header_buf[5]; // One more byte than needed.
	unsigned char integer_buf[4];
	double sample_buf, sign_bias, max_abs_value, cur_amp_sum;
	short int num_channels;
	int cur_amp_num, cur_amp_index;
	int bytes_per_sample;
	int offset_ms = 0;
	int cutoff_ms = 0;
	double *waveform; // Output goes here.

	header_buf[4] = '\0'; // Needs to be null-terminated for strcmp to work.
	file = fopen(filename, "rb");
	if(NULL == file) 
	{
		fprintf(stderr, "Error while loading file.\n");
		return NULL;
	}
	
	// Check headers.
	size_t read_result = fread(header_buf, sizeof(char), 4, file); // "RIFF"
	assert(read_result == 4);
	if(0 != strcmp(header_buf,"RIFF"))
	{
		fclose(file);
		fprintf(stderr, "Error: Missing RIFF header in input file.\n");
		return NULL;
	}
	fseek(file, 4, SEEK_CUR); // Ignore the file size.
	read_result = fread(header_buf, sizeof(char), 4, file); // "WAVE"
	assert(read_result == 4);
	if(0 != strcmp(header_buf,"WAVE"))
	{
		fclose(file);
		fprintf(stderr, "Error: Missing WAVE header in input file.\n");
		return NULL;
	}
	read_result = fread(header_buf, sizeof(char), 4, file); // "fmt "
	assert(read_result == 4);
	if(0 != strcmp(header_buf,"fmt "))
	{
		fclose(file);
		fprintf(stderr, "Error: Missing fmt header in input file.\n");
		return NULL;
	}
	read_result = fread(header_buf, sizeof(char), 4, file); //1 0 0 0
	assert(read_result == 4);
	if(!(16 == header_buf[0] && 0 == header_buf[1] && 0 == header_buf[2] && 0 == header_buf[3]))
	{
		fclose(file);
		fprintf(stderr, "Error: Missing or incorrect fmt length in input file.\n");
		return NULL;
	}
	read_result = fread(header_buf, sizeof(char), 2, file); //1 0
	assert(read_result == 2);
	if(!(1 == header_buf[0] && 0 == header_buf[1]))
	{
		fclose(file);
		fprintf(stderr, "Error: Missing format ID in input file.\n");
		return NULL;
	}

	// Can only handle one channel for now.  Ignores channels besides the first.
	read_result = fread(integer_buf, sizeof(char), 2, file);
	assert(read_result == 2);
	num_channels = integer_buf[0]; // We don't want more than 256 channels.
	if (num_channels > 1) {
		fprintf(stderr, "Warning: Channels after the first will be ignored.\n");
	}

	// Sampling frequency in Hz.
	read_result = fread(integer_buf, sizeof(char), 4, file);
	assert(read_result == 4);
	*sample_rate = 0;
	for(int i = 3;i >= 0;i--)
	{
		*sample_rate = (*sample_rate * 256) + integer_buf[i];
	}
	
	// Bits per sample.
	fseek(file, 6, SEEK_CUR); // Skip bytes/second and bytes/sample
	read_result = fread(integer_buf, sizeof(char), 2, file);
	assert(read_result == 2);
	*bits_per_sample = integer_buf[0]; // Should be a multiple of 8.
	bytes_per_sample = *bits_per_sample / 8;
	if (*bits_per_sample % 8 > 0) bytes_per_sample++; // Round up if necessary.

	// Skip through the file until finding the required data header.
	int chunk_length;
	read_result = fread(header_buf, sizeof(char), 4, file); // "data"?
	assert(read_result == 4);
	while(strcmp(header_buf,"data") != 0)
	{
		read_result = fread(&chunk_length, sizeof(char), 4, file);
		assert(read_result == 4);
		fseek(file, chunk_length, SEEK_CUR); // Skip over unimportant chunk.
		read_result = fread(header_buf, sizeof(char), 4, file); // "data">
		assert(read_result == 4);
	}

	// Number of samples.
	read_result = fread(integer_buf, sizeof(char), 4, file);
	assert(read_result == 4);
	*num_samples = 0;
	for(int i = 3;i >= 0;i--)
	{
		*num_samples = (*num_samples * 256) + integer_buf[i]; // Total bytes of data.
	}
	*num_samples /= (bytes_per_sample * num_channels); // Divide by bytes per sample.

	if(cutoff_ms < 0) // Negative cutoff->add to offset instead of subtracting from end.
	{
		cutoff_ms = (*num_samples * 1000.0 / *sample_rate) - (offset_ms - cutoff_ms);
	}

	int start, end; // First and last sample index.

	// Number of samples in offset (100 ms smaller) boxed by min and max samples.
	start = max(0, min(*num_samples-1, (int)((offset_ms-100) * *sample_rate / 1000.0))); 

	// Number of samples minus number of samples in cutoff (100 ms smaller) if > 0,
	// boxed by min and max samples.
	end = max(0, min(
		*num_samples-1,
		*num_samples - (int)(max(0, cutoff_ms - 100) * *sample_rate / 1000.0)));
		
	// Cutoff and offset should both be set to about 100 ms.
	cutoff_ms = (int) ((end * 1000.0 / *sample_rate) - 
			((*num_samples * 1000.0 / *sample_rate) - cutoff_ms));
	offset_ms = offset_ms - (int) (start * 1000.0 / *sample_rate);
	*num_samples = (end - start + 1); // Shorten number of samples to only those read.
	*num_amp_values = (int)(*num_samples * 1.0 / samples_per_frq) + 1;
	
	// Get the actual waveform (signal).
	waveform = (double *)malloc(sizeof(double) * *num_samples);
	*avg_amp = (double *)malloc(sizeof(double) * *num_amp_values);
	if (waveform == NULL) { return NULL; }
	if (*avg_amp == NULL) { return NULL; }

	// Skip to the start byte of signal.
	fseek(file, start * bytes_per_sample * num_channels, SEEK_CUR);

	size_t total_num_bytes = *num_samples * bytes_per_sample * num_channels;
	unsigned char *wav_buf = (unsigned char *) malloc(sizeof(char) * total_num_bytes);

	// Read everything else from the file into memory.
	read_result = fread(wav_buf, sizeof(char), total_num_bytes, file);
	assert(read_result == total_num_bytes);

	// Max absolute value of a sample.
	max_abs_value = pow(2.0, *bits_per_sample-1);
	
	// Keeps track of the current amplitude sum.
	cur_amp_sum = 0.0;
	cur_amp_num = 0;
	cur_amp_index = 0;

	int cur_byte;
	for(int cur_sample = 0; cur_sample < *num_samples; cur_sample++)
	{
		cur_byte = cur_sample * bytes_per_sample * num_channels;
		sign_bias = 0.0;
		sample_buf = 0.0;

		// If sample is negative, set the sign bias and flip to positive.
		if(wav_buf[cur_byte + bytes_per_sample-1] >= 128)
		{
			sign_bias = pow(2.0, *bits_per_sample-1);
			wav_buf[cur_byte + bytes_per_sample-1] &= 0x7F;
		}
		// Reading the data.
		for(int j = bytes_per_sample-1;j >= 0;j--)
		{
			// Extract sample as an unsigned int, save as double.
			sample_buf = (sample_buf * 256.0) + (double)(wav_buf[cur_byte + j]);
		}
		// Add value to amplitude sum and average if necessary.
		cur_amp_sum += abs(sample_buf - sign_bias);
		cur_amp_num++;
		if ((cur_amp_num >= samples_per_frq) || (cur_sample == *num_samples - 1)) {
			if (cur_amp_index >= *num_amp_values) {
				fprintf(stderr, "Warning: Too many amplitudes calculated.\n");
			}
			(*avg_amp)[cur_amp_index] = cur_amp_sum / cur_amp_num;
			cur_amp_sum = 0.0;
			cur_amp_num = 0;
			cur_amp_index++;
		}
		// Normalize each sample to [-1,1).
		waveform[cur_sample] = ((double)(sample_buf - sign_bias) / max_abs_value);
	}

	// Clean up.
	free(wav_buf);
	fclose(file);
	return waveform;
}
