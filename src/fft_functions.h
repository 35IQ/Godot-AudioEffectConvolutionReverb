<<<<<<< HEAD
#pragma once
#include <cmath>
#include <immintrin.h>
#include "signal.h"

/*
inline void smbFftOptimized(float *fftBuffer, long fftFrameSize, TwiddleFactorsComplex * twiddleFactors, long sign) {
	int i, j, k;
	const int n = fftFrameSize << 1;

	// Bit-reversal permutation
	j = 0;
	for (i = 0; i < n; i += 2) {
		if (j > i) {
			std::swap(fftBuffer[i], fftBuffer[j]);
			std::swap(fftBuffer[i + 1], fftBuffer[j + 1]);
		}
		int m = fftFrameSize;
		while (m >= 2 && j >= m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	// FFT Part
	int stages = ctz32(fftFrameSize);
	int step = 2;

	for(i = 0; i < stages; ++i) {
		int leap = step << 1;

		// float ang = sign * Math_PI / (step >> 1);
		// float wtemp = sinf(0.5f * ang);

		// float wpr = -2.0f * wtemp * wtemp;
		// float wpi = sinf(ang);
		int index = n / (step << 1);
		float wpr = twiddleFactors->data[2 * index + 0]; // real part - 1
		float wpi = sign * twiddleFactors->data[2 * index + 1]; // imaginary part

		float wr = 1.0f;
		float wi = 0.0f;

		for(j = 0; j < step; j += 2) {
			for(k = j; k < n; k += leap) {
				int match = k + step;

				float ur = fftBuffer[k];
				float ui = fftBuffer[k + 1];

				float tempr = wr * fftBuffer[match] - wi * fftBuffer[match + 1];
				float tempi = wr * fftBuffer[match + 1] + wi * fftBuffer[match];

				fftBuffer[k] = ur + tempr;
				fftBuffer[k + 1] = ui + tempi;

				fftBuffer[match] = ur - tempr;
				fftBuffer[match + 1] = ui - tempi;
			}

			float wtemp_wr = wr;
			wr += wtemp_wr * wpr - wi * wpi;
			wi += wtemp_wr * wpi + wi * wpr;
		}
		step = leap;
	}
}
*/


inline void smbFftSoA(float *real, float *imag, int frameCount, int sign){
   int i, j, k, le, le2;
    int stages = (int)(log((double)frameCount) / log(2.0) + 0.5);
    float ur, ui, wr, wi, tr, ti;

    // -------- Bit reversal --------
    for (i = 0; i < frameCount; i++) {
        j = 0;
        int x = i;
        for (int bit = 0; (1 << bit) < frameCount; bit++) {
            j = (j << 1) | (x & 1);
            x >>= 1;
        }
        if (i < j) {
            std::swap(real[i], real[j]);
            std::swap(imag[i], imag[j]);
        }
    }

    // -------- FFT stages --------
    for (k = 0, le = 1; k < stages; k++) {
        le <<= 1;
        le2 = le >> 1;
        ur = 1.0f;
        ui = 0.0f;
        float arg = Math_PI / le2;
        wr = cosf(arg);
        wi = sign * sinf(arg);

        for (j = 0; j < le2; j++) {
            for (i = j; i < frameCount; i += le) {
                int ip = i + le2;
                tr = real[ip] * ur - imag[ip] * ui;
                ti = real[ip] * ui + imag[ip] * ur;

                real[ip] = real[i] - tr;
                imag[ip] = imag[i] - ti;
                real[i] += tr;
                imag[i] += ti;
            }
            tr = ur * wr - ui * wi;
            ui = ur * wi + ui * wr;
            ur = tr;
        }
    }
}


inline void smbFft(float *fftBuffer, long fftFrameSize, long sign)
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

// __forceinline void ComplexMultiplyAccumulateAVX( const float* Xi, const float* Hi, float* accum, int fftDataSize){
// 	int simdEnd = (fftDataSize / 8) * 8;
//     for (int i = 0; i < simdEnd; i += 8) {
// 		__m256 x = _mm256_load_ps(Xi + i);
// 		__m256 h = _mm256_load_ps(Hi + i);
// 		__m256 xr = _mm256_moveldup_ps(x);
// 		__m256 xi = _mm256_movehdup_ps(x);
// 		__m256 h_sw = _mm256_shuffle_ps(h, h, 0xB1);
// 		__m256 t1 = _mm256_mul_ps(xr, h); 
// 		__m256 t2 = _mm256_mul_ps(xi, h_sw);
// 		__m256 result = _mm256_addsub_ps(t1, t2);
// 		__m256 acc = _mm256_load_ps(accum + i);
// 		acc = _mm256_add_ps(acc, result);
// 		_mm256_store_ps(accum + i, acc);
// 	}
// 	for (int i = simdEnd; i < fftDataSize; i += 2){
//         float r = Xi[i]*Hi[i] - Xi[i+1]*Hi[i+1];
//         float im = Xi[i]*Hi[i+1] + Xi[i+1]*Hi[i];
//         accum[i] += r;
//         accum[i+1] += im;
//     }
// }

// __forceinline void ComplexMultiplyFirstPartitionAVX(const float* Xi, const float* Hi, float* accum, int fftDataSize){
// 	int simdEnd = (fftDataSize / 8) * 8;
//     for (int i = 0; i < simdEnd; i += 8) {
// 		__m256 x = _mm256_load_ps(Xi + i);
// 		__m256 h = _mm256_load_ps(Hi + i);
// 		__m256 xr = _mm256_moveldup_ps(x);
// 		__m256 xi = _mm256_movehdup_ps(x);
// 		__m256 h_sw = _mm256_shuffle_ps(h, h, 0xB1);
// 		__m256 t1 = _mm256_mul_ps(xr, h); 
// 		__m256 t2 = _mm256_mul_ps(xi, h_sw);
// 		__m256 result = _mm256_addsub_ps(t1, t2);
// 		_mm256_store_ps(accum + i, result);
// 	}
// 	for (int i = simdEnd; i < fftDataSize; i += 2){
//         float r = Xi[i]*Hi[i] - Xi[i+1]*Hi[i+1];
//         float im = Xi[i]*Hi[i+1] + Xi[i+1]*Hi[i];
//         accum[i] = r;
//         accum[i+1] = im;
//     }
// }


__forceinline void ComplexMultiplyFirstPartition(const iqAudioSignalComplex* Xi, const iqAudioSignalComplex* Hi, iqAudioSignalComplex* accum, int fftDataSize) {
    for (int i = 0; i < fftDataSize; i ++) {
        accum->real[i] = Xi->real[i] * Hi->real[i] - Xi->imag[i] * Hi->imag[i];
        accum->imag[i] = Xi->real[i] * Hi->imag[i] + Xi->imag[i] * Hi->real[i];
    }
}

__forceinline void ComplexMultiplyAccumulate(const iqAudioSignalComplex* Xi, const iqAudioSignalComplex* Hi, iqAudioSignalComplex* accum, int fftDataSize){
		for (int i = 0; i < fftDataSize; i ++) {
			accum->real[i] += Xi->real[i] * Hi->real[i] - Xi->imag[i] * Hi->imag[i];
			accum->imag[i] += Xi->real[i] * Hi->imag[i] + Xi->imag[i] * Hi->real[i];
		}
}

__forceinline void ComplexMultiplyFirstPartitionAVX(
    const iqAudioSignalComplex* __restrict Xi,
    const iqAudioSignalComplex* __restrict Hi,
    iqAudioSignalComplex* __restrict accum,
    int fftDataSize)
{
    const float* __restrict xr_ptr = Xi->real.data();
    const float* __restrict xi_ptr = Xi->imag.data();
    const float* __restrict hr_ptr = Hi->real.data();
    const float* __restrict hi_ptr = Hi->imag.data();

    float* __restrict ar_ptr = accum->real.data();
    float* __restrict ai_ptr = accum->imag.data();

    int simdEnd = fftDataSize & ~7;

    for (int i = 0; i < simdEnd; i += 8)
    {
        __m256 xr0 = _mm256_load_ps(xr_ptr + i);
        __m256 xi0 = _mm256_load_ps(xi_ptr + i);
        __m256 hr0 = _mm256_load_ps(hr_ptr + i);
        __m256 hi0 = _mm256_load_ps(hi_ptr + i);

        __m256 xi_hi0 = _mm256_mul_ps(xi0, hi0);
        __m256 xi_hr0 = _mm256_mul_ps(xi0, hr0);

        __m256 real0 = _mm256_fmsub_ps(xr0, hr0, xi_hi0);
        __m256 imag0 = _mm256_fmadd_ps(xr0, hi0, xi_hr0);

        _mm256_store_ps(ar_ptr + i, real0);
        _mm256_store_ps(ai_ptr + i, imag0);

    }

    for (int i = simdEnd; i < fftDataSize; i++)
    {
        const float xr = xr_ptr[i];
        const float xi = xi_ptr[i];
        const float hr = hr_ptr[i];
        const float hi = hi_ptr[i];

        ar_ptr[i] = xr*hr - xi*hi;
        ai_ptr[i] = xr*hi + xi*hr;
    }
}

__forceinline void ComplexMultiplyAccumulateAVX(
    const iqAudioSignalComplex* __restrict Xi,
    const iqAudioSignalComplex* __restrict Hi,
    iqAudioSignalComplex* __restrict accum,
    int fftDataSize)
{
    const float* __restrict xr_ptr = Xi->real.data();
    const float* __restrict xi_ptr = Xi->imag.data();
    const float* __restrict hr_ptr = Hi->real.data();
    const float* __restrict hi_ptr = Hi->imag.data();

    float* __restrict ar_ptr = accum->real.data();
    float* __restrict ai_ptr = accum->imag.data();

    int simdEnd = fftDataSize & ~7;

    for (int i = 0; i < simdEnd; i += 8)
    {
        __m256 xr = _mm256_load_ps(xr_ptr + i);
        __m256 xi = _mm256_load_ps(xi_ptr + i);
        __m256 hr = _mm256_load_ps(hr_ptr + i);
        __m256 hi = _mm256_load_ps(hi_ptr + i);

        __m256 acc_r = _mm256_load_ps(ar_ptr + i);
        __m256 acc_i = _mm256_load_ps(ai_ptr + i);

        __m256 xi_hi = _mm256_mul_ps(xi, hi);
        __m256 xi_hr = _mm256_mul_ps(xi, hr);

        __m256 real = _mm256_fmsub_ps(xr, hr, xi_hi);
        __m256 imag = _mm256_fmadd_ps(xr, hi, xi_hr);

        acc_r = _mm256_add_ps(acc_r, real);
        acc_i = _mm256_add_ps(acc_i, imag);

        _mm256_store_ps(ar_ptr + i, acc_r);
        _mm256_store_ps(ai_ptr + i, acc_i);
    }

    for (int i = simdEnd; i < fftDataSize; i++)
    {
        const float xr = xr_ptr[i];
        const float xi = xi_ptr[i];
        const float hr = hr_ptr[i];
        const float hi = hi_ptr[i];
        ar_ptr[i] += xr*hr - xi*hi;
        ai_ptr[i] += xr*hi + xi*hr;
    }
=======
#pragma once
#include <cmath>
#include <immintrin.h>


inline void smbFft(float *fftBuffer, long fftFrameSize, long sign)
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

inline void ComplexMultiplyAccumulateAVX( const float* Xi, const float* Hi, float* accum, int fftDataSize){
	int simdEnd = (fftDataSize / 8) * 8;
    for (int i = 0; i < simdEnd; i += 8) {
		__m256 x = _mm256_load_ps(Xi + i);
		__m256 h = _mm256_load_ps(Hi + i);
		__m256 xr = _mm256_moveldup_ps(x);
		__m256 xi = _mm256_movehdup_ps(x);
		__m256 h_sw = _mm256_shuffle_ps(h, h, 0xB1);
		__m256 t1 = _mm256_mul_ps(xr, h); 
		__m256 t2 = _mm256_mul_ps(xi, h_sw);
		__m256 result = _mm256_addsub_ps(t1, t2);
		__m256 acc = _mm256_load_ps(accum + i);
		acc = _mm256_add_ps(acc, result);
		_mm256_store_ps(accum + i, acc);
	}
	for (int i = simdEnd; i < fftDataSize; i += 2){
        float r = Xi[i]*Hi[i] - Xi[i+1]*Hi[i+1];
        float im = Xi[i]*Hi[i+1] + Xi[i+1]*Hi[i];
        accum[i] += r;
        accum[i+1] += im;
    }
}

inline void ComplexMultiplyAccumulate(const float* Xi, const float* Hi, float* accum, int fftDataSize){
		for (int i = 0; i < fftDataSize; i += 2) {
			float r = Xi[i] * Hi[i] - Xi[i+1] * Hi[i+1];
			float im = Xi[i] * Hi[i+1] + Xi[i+1] * Hi[i];
			accum[i] += r;
			accum[i+1] += im;
		}
>>>>>>> f4ff7c7e760ef902c0dd87dabebf1cf6bef0a404
}