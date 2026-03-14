#pragma once

#if defined(_MSC_VER)
#include <intrin.h>
#endif
#include <cmath>
#include <vector>
#include "vector_alloc.h"
//Class for FFT, adapted from the original code by Stefan Bernsee (smb) with optimizations and modifications for this project.

#ifndef Math_PI
#define Math_PI 3.14159265358979323846f
#endif

inline int ctz32( unsigned int x ) {
#if defined( _MSC_VER ) 
    unsigned long idx = 0;
    _BitScanForward( &idx, x ); // x must be non-zero
    return static_cast<int>( idx );
#else
    return __builtin_ctz( x );  // x must be non-zero
#endif
}

class iqFFT {
	public:	
	private:
		std::vector<float, AlignedAllocator<float, 32>> twiddleFactors;	
		std::vector<std::pair<int,int>> bitrevSwaps;

		int complexSize =-1; 
		int N;
		int stages;

	public:
		//constructor
		iqFFT(){
			SetUpFFT( 1024 );
		}
		~iqFFT() = default;
		void SetUpFFT( int pComplexSize );
		void FFT( float* fftBuffer, const int sign );
};	

