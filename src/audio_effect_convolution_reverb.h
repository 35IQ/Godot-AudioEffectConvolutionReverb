#pragma once
#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_frame.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>

#include "fft_functions.h"
#include "vector_alloc.h"

namespace godot{
	class AudioEffectConvolutionReverb;
	class AudioEffectConvolutionReverbInstance : public AudioEffectInstance{
		GDCLASS(AudioEffectConvolutionReverbInstance, AudioEffectInstance);
		friend class AudioEffectConvolutionReverb;

		Ref<AudioEffectConvolutionReverb> base;	
		int blockSize = 0;
		int fftSize = 0;
		float invFFT;
		int fftDataSize = 0;
		int overlapSize = 0;
		int partitionCount = 0;
		int inputPos = 0;

		// FFT of impulse response partitions
		std::vector<std::vector<float, AlignedAllocator<float, 32>>> impulseLeftFFT;
		std::vector<std::vector<float, AlignedAllocator<float, 32>>> impulseRightFFT;
		// FFT of input blocks
		std::vector<std::vector<float, AlignedAllocator<float, 32>>> inputLeftFFT;
		std::vector<std::vector<float, AlignedAllocator<float, 32>>> inputRightFFT;
		// accumulator	
		std::vector<float, AlignedAllocator<float, 32>> accumLeft;
		std::vector<float, AlignedAllocator<float, 32>> accumRight;
		// overlap
		std::vector<float, AlignedAllocator<float, 32>> overlapLeft;
		std::vector<float, AlignedAllocator<float, 32>> overlapRight;

	protected:
		static void _bind_methods(){};

	public:
		virtual void _process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) ;

	private:
		bool SetImpulseResponse(const int p_frame_count);
	};

	class AudioEffectConvolutionReverb : public AudioEffect {
		GDCLASS(AudioEffectConvolutionReverb, AudioEffect);
		friend class AudioEffectConvolutionReverbInstance;
		using ComplexMulFn = void(*)(const float* p_src, const float* p_imp, float* p_out, int fftDataSize);

		protected:		
		String impulseResponsePath;
		Ref<AudioStreamWAV> impulse;

		bool update = false;
		float time = 0.0f;
		float mix = 1.0f;
		float gain = 1.0f;
		bool useAVX = false;
		ComplexMulFn complexMulAccumulate = ComplexMultiplyAccumulate;
		static void _bind_methods();

	public:
		virtual Ref<AudioEffectInstance> _instantiate();
		String GetImpulseResponsePath() const;
		void SetImpulseResponsePath(const String &p_impulse_response_path);
		void SetMix(float p_mix);
		float GetMix() const;
		void SetUseAVX(bool p_use_avx);
		bool GetUseAVX() const;
		void SetGain(float p_gain);
		float GetGain() const;
		float GetProcessTime() const;
	};

}
