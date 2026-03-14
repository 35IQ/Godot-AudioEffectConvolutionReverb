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
			print_line("CTZ: " + String::num(ctz32(1024)) );
		}
		
		float GetProcessTime() const { return time; }
	};

}
