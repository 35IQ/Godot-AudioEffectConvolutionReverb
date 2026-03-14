#pragma once
#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>

#define Math_PI_2 1.5707963267948966192313216916398
#include "fft.h"
#include "vector_alloc.h"
#include "fft_functions.h"

struct Buffer{
	std::vector<float, AlignedAllocator<float, 32>> left;
	std::vector<float, AlignedAllocator<float, 32>> right;
};

namespace godot{
	class AudioEffectConvolutionReverb;
	class AudioEffectConvolutionReverbInstance : public AudioEffectInstance{
		GDCLASS(AudioEffectConvolutionReverbInstance, AudioEffectInstance);
		friend class AudioEffectConvolutionReverb;

		Ref<AudioEffectConvolutionReverb> base;	
		iqFFT fft;
		int blockSize = 512;
		int complexLength = 1024;
		int fftSize = 2048;
		int overlapSize = 1024;
		int partitionCount = 0;
		int inputPos = 0;
		char currentAccum = 0;
		float invFFT;

		// FFT of impulse response partitions
		std::vector<Buffer> impulseFD;
		// Input blocks
		std::vector<Buffer> input;
		// Accumulator	
		Buffer accumulators[2];

	protected:
		static void _bind_methods(){};

	public:
		virtual void _process( const AudioFrame *pSrcFrames, AudioFrame *pDstFrames, int pFrameCount ) ;

	private:
		bool SetImpulseResponse( const int pFrameCount );
		void ComplexMultiplyFirstPartition( const float*__restrict pSignal, const float*__restrict pImpulse, float*__restrict pAccum, int pSize );
		void ComplexMultiplyAccumulate( const float*__restrict pSignal, const float*__restrict pImpulse, float*__restrict pAccum, int pSize );
	};

	class AudioEffectConvolutionReverb : public AudioEffect {
		GDCLASS(AudioEffectConvolutionReverb, AudioEffect);
		friend class AudioEffectConvolutionReverbInstance;

	protected:		
		String impulseResponsePath;
		Ref<AudioStreamWAV> impulse;
		bool test = false;
		bool update = false;
		float time = 0.0f;
		float mix = 1.0f;
		float dry, wet;
		float gain = 1.0f;
		static void _bind_methods();
			

	public:
		virtual Ref<AudioEffectInstance> _instantiate();

		String GetImpulseResponsePath() const{ return impulseResponsePath; }
		void SetImpulseResponsePath(const String &pImpulseResponsePath);

		float GetMix() const{ return mix; }
		void SetMix( float pMix ){ mix = pMix; dry = cos( mix * Math_PI_2 ); wet = sin( mix * Math_PI_2 ); }

		float GetGain() const { return gain; }
		void SetGain( float pGain ) { gain = pGain; }
	
		bool GetTest() const { return test; }
		void SetTest(bool pTest) {
			test = pTest;
			// ctz32(1024); 
			// if ( !test ) return;
			print_line("CTZ: " + String::num(ctz32(1024)) );
			// float testBuffer[8] = { 1.0, 0.0, 0.0, 4.0, 0.0, 0.0, 23.0, 0.0 };
			// iqAudioSignalComplex testReal;
			// testReal.real = { 1.0f, 0.0f, 0.0f, 23.0f };
			// testReal.imag = { 0.0f, 4.0f, 0.0f, 0.0f };
			// smbFft(testBuffer, 4, 1);
			// // fft.useOldFFT();
			// fft.run(&testReal, 4, 1);

			// print_line("aos FFT output:");
			// for ( int i = 0; i < 8; i += 2) {
			// 	print_line("Real: " + String::num(testBuffer[i]) + " Imag: " + String::num(testBuffer[i + 1]));
			// }
			// print_line("soa FFT output:");
			// for (int i = 0; i < 4; i++) {
			// 	print_line("Real: " + String::num(testReal.real[i]) + " Imag: " + String::num(testReal.imag[i]));
			// }
			
		}
		
		float GetProcessTime() const { return time; }
	};

}
