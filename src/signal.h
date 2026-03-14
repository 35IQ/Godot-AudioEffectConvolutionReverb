#pragma once
#include <vector>
#include "vector_alloc.h"
//soa signal struct for complex numbers, used for FFT buffers and operations
struct iqAudioSignalComplex{

	std::vector<float, AlignedAllocator<float, 32>> real; // real parts
	std::vector<float, AlignedAllocator<float, 32>> imag; // imaginary parts

	iqAudioSignalComplex() {}
	iqAudioSignalComplex(size_t size) {
		Resize(size);
	}

	void Resize(size_t newSize) {
		real.resize(newSize, 0.0f);
		imag.resize(newSize, 0.0f);
	}

	void Clear() {
		real.clear();
		imag.clear();
	}
};