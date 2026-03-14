#include "fft.h"
#include "math.h"

void iqFFT::SetUpFFT( int fftFrameSize ) {
	if ( fftFrameSize == complexSize ) { return; } 
	complexSize = fftFrameSize; 
	stages = ctz32(complexSize); 
	N = complexSize * 2;

// Precompute twiddle factors 
	twiddleFactors.resize(complexSize); 
	for (int i = 0; i < complexSize / 2; i++) { 
		float angle = 2.0f * Math_PI * i / complexSize; 
		float wtemp = sinf(0.5f * angle); 
		twiddleFactors[2 * i + 0] = cosf(angle) - 1.0f; // real part - 1 
		twiddleFactors[2 * i + 1] = sinf(angle); // imaginary part 
	}
	
	// Precompute bitrev swaps for optimized FFT	
	int n = complexSize; // количество комплексных чисел
	std::vector<int> bitrev;
	bitrev.resize(N); // real+imag
	int j = 0;
	for (int i = 0; i < n; i++) {
		int rev = 0;
		int x = i;
		for (int k = 0; (1 << k) < n; k++) {
			rev = (rev << 1) | (x & 1);
			x >>= 1;
		}
			bitrev[i*2] = rev*2;     // real
			bitrev[i*2+1] = rev*2+1; // imag
	}	
	
	bitrevSwaps.clear();
	for (int i = 0; i < n; i++) {
		int rev = bitrev[i*2]; // real index
		if (i < rev/2) { // Ensure we only swap once
			bitrevSwaps.emplace_back(i*2, rev); 
		}
	}
}	 

void iqFFT::smbFft(float *fftBuffer, long fftFrameSize, int sign)
/*
	FFT routine, (C)1996 S.M.Bernsee. Sign = -1 is FFT, 1 is iFFT (inverse)
	Fills fftBuffer[0...2*fftFrameSize-1] with the Fourier transform of the
	time domain data in fftBuffer[0...2*fftFrameSize-1]. The FFT array takes
	and returns the cosine and sine parts in an interleaved manner, ie.
	fftBuffer[0] = cosPart[0], fftBuffer[1] = sinPart[0], asf. fftFrameSize
	must be a power of 2. It expects a complex input signal (see footnote 2),
	ie. when working with 'common' audio signals our input signal has to be
	passed as {in[0],0.,in[1],0.,in[2],0.,...} asf. In that case, the transform
	of the frequencies of interest is in fftBuffer[0...fftFrameSize].
*/
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(log((double)fftFrameSize) / log(2.) + .5); k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = Math_PI / (le2 >> 1);
		wr = cos(arg);
		wi = sign * sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}


void iqFFT::FFT( float* fftBuffer, const int sign ) {
	int i, j, k;
	//buffer Bit Reverse
	float rB, iB;
	for( auto [idx1, idx2] : bitrevSwaps ) {
		rB = fftBuffer[idx1];
		iB = fftBuffer[idx1 + 1];
		fftBuffer[idx1] = fftBuffer[idx2];
		fftBuffer[idx1 + 1] = fftBuffer[idx2 + 1];
		fftBuffer[idx2] = rB;
		fftBuffer[idx2 + 1] = iB;
	}

	// FFT Part
	int step = 2;
	for( i = 0; i < stages; ++i ) {
		int leap = step << 1;
		int idxR = N / ( step << 1 ) << 1; 
		int idxI = idxR + 1; 
		float wpR = twiddleFactors[idxR]; 
		float wpI = sign * twiddleFactors[idxI]; 
		float wR = 1.0f;
		float wI = 0.0f;
		for( j = 0; j < step; j += 2 ) {
			for( k = j; k < N; k += leap ) {
				int matchR = k + step;
				int matchI = matchR + 1;
				float uR = fftBuffer[k];
				float uI = fftBuffer[k + 1];

				float tempR = wR * fftBuffer[matchR] - wI * fftBuffer[matchI];
				float tempI = wR * fftBuffer[matchI] + wI * fftBuffer[matchR];

				fftBuffer[k] = uR + tempR;
				fftBuffer[k + 1] = uI + tempI;

				fftBuffer[matchR] = uR - tempR;
				fftBuffer[matchI] = uI - tempI;
			}
			float tempWR = wR;
			wR += tempWR * wpR - wI * wpI;
			wI += tempWR * wpI + wI * wpR;
		}
		step = leap;
	}
}
