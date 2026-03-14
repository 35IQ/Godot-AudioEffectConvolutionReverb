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
	int n = complexSize;
	std::vector<int> bitrev;
	bitrev.resize(N); 
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
		if (i < rev/2) {
			bitrevSwaps.emplace_back(i*2, rev); 
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
