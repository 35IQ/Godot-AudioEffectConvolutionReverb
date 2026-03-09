#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include "audio_effect_convolution_reverb.h"

using namespace godot;
#define Math_PI_2 1.5707963267948966192313216916398

void AudioEffectConvolutionReverbInstance::_process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count){
	int T = Time::get_singleton()->get_ticks_usec();
	if (base->update) {
		base->update = false;
		SetImpulseResponse(p_frame_count);
		print_line("Impulse response updated.");
	}
	//bypass
	if ( partitionCount == 0 ) {	
		std::memcpy(p_dst_frames, p_src_frames, p_frame_count * sizeof(AudioFrame));
		return;	
	}
	auto &srcL = inputLeftFFT[inputPos];
	auto &srcR = inputRightFFT[inputPos];
	for (int i = 0; i < blockSize; i++) {
		srcL[i*2] = p_src_frames[i].left;
		srcL[i*2+1] = 0;
		srcR[i*2] = p_src_frames[i].right;
		srcR[i*2+1] = 0;
	}
	for (int i = blockSize*2; i < fftDataSize; i++) {
		srcL[i] = 0;
		srcR[i] = 0;
	}
	smbFft(srcL.data(), fftSize, -1);
	smbFft(srcR.data(), fftSize, -1);
	std::fill(accumLeft.begin(), accumLeft.end(), 0.0f);
	std::fill(accumRight.begin(), accumRight.end(), 0.0f);
	// convolution sum
	for (int p = 0; p < partitionCount; p++) {
		int idx = inputPos - p;
		if (idx < 0) idx += partitionCount;
		auto &XiL = inputLeftFFT[idx];
		auto &XiR = inputRightFFT[idx];
		auto &HiL = impulseLeftFFT[p];
		auto &HiR = impulseRightFFT[p];
		base->complexMulAccumulate(XiL.data(), HiL.data(), accumLeft.data(), fftDataSize);
		base->complexMulAccumulate(XiR.data(), HiR.data(), accumRight.data(), fftDataSize);
	}
	smbFft(accumLeft.data(), fftSize, 1);
	smbFft(accumRight.data(), fftSize, 1);
	// overlap-add
	for (int i = 0; i < blockSize; i++) {
		const float outL = (accumLeft[i*2] * invFFT + overlapLeft[i]) * base->gain;
		const float outR = (accumRight[i*2] * invFFT + overlapRight[i]) * base->gain;
		p_dst_frames[i].left = 	p_src_frames[i].left * Math::cos(base->mix * Math_PI_2) + outL * Math::sin(base->mix * Math_PI_2);
		p_dst_frames[i].right = 	p_src_frames[i].right * Math::cos(base->mix * Math_PI_2) + outR * Math::sin(base->mix * Math_PI_2);
	}

	for (int i = 0; i < blockSize; i++) {
		overlapLeft[i] = accumLeft[(i + blockSize)*2] * invFFT;
		overlapRight[i] = accumRight[(i + blockSize)*2] * invFFT;
	}

	inputPos++;
	if (inputPos >= partitionCount)
		inputPos = 0;
	base->time = float( Time::get_singleton()->get_ticks_usec() - T ) * 0.000001;
}

bool AudioEffectConvolutionReverbInstance::SetImpulseResponse(const int p_frame_count){
	if (!base.is_valid() || !base->impulse.is_valid()) {
		print_error("Impulse response = NULL");
		return false;
	}
	PackedByteArray pcm = base->impulse.ptr()->get_data();
	const int pcmSize = pcm.size();

	if (pcmSize == 0) {
		print_error("Impulse response PCM data is empty.");
		return false;
	}

	std::vector<float> impulseLeft;
	std::vector<float> impulseRight;
	if (base->impulse.ptr()->get_format() == AudioStreamWAV::FORMAT_8_BITS) {
		for (int i = 0; i < pcmSize ; i += 2){
			impulseLeft.push_back (float(pcm.decode_s8(i) 	 ) / 128.0f);
			impulseRight.push_back(float(pcm.decode_s8(i + 1)) / 128.0f);
		}
	} else if(base->impulse.ptr()->get_format() == AudioStreamWAV::FORMAT_16_BITS){
		for (int i = 0; i < pcmSize ; i += 4){
			impulseLeft.push_back (float(pcm.decode_s16(i) 	  ) / 32768.0f);
			impulseRight.push_back(float(pcm.decode_s16(i + 2)) / 32768.0f);
		}
	} else {
		print_error("Unsupported audio format for impulse response. Only 8-bit and 16-bit PCM WAV files are supported.");
		return false;
	}

	blockSize = p_frame_count;
	fftSize = blockSize * 2;
	invFFT = 1.0f / float(fftSize);
	fftDataSize = fftSize * 2;

	int impulseSize = impulseLeft.size();
	partitionCount = (impulseSize + blockSize - 1) / blockSize;
	print_line("Impulse samples: " + String::num(impulseSize));
	print_line("Partitions: " + String::num(partitionCount));

	impulseLeftFFT.clear();
	impulseRightFFT.clear();
	inputLeftFFT.clear();
	inputRightFFT.clear();

	impulseLeftFFT.resize(partitionCount);
	impulseRightFFT.resize(partitionCount);

	inputLeftFFT.resize(partitionCount);
	inputRightFFT.resize(partitionCount);
	
	for (int p = 0; p < partitionCount; p++) {
		inputLeftFFT[p].resize(fftDataSize, 0.0f);
		inputRightFFT[p].resize(fftDataSize, 0.0f);

		impulseLeftFFT[p].resize(fftDataSize, 0.0f);
		impulseRightFFT[p].resize(fftDataSize, 0.0f);

		for(int i = 0; i < blockSize; i++) {
			int idx = p * blockSize + i;
			if (idx < impulseSize) {
				impulseLeftFFT[p][i*2] = impulseLeft[idx];
				impulseRightFFT[p][i*2] = impulseRight[idx];
			}
		}
		smbFft(impulseLeftFFT[p].data(), fftSize, -1);
		smbFft(impulseRightFFT[p].data(), fftSize, -1);
	}

	accumLeft.resize(fftDataSize, 0.0f);
	accumRight.resize(fftDataSize, 0.0f);
	overlapLeft.resize(blockSize, 0.0f);
	overlapRight.resize(blockSize, 0.0f);
	inputPos = 0;
	return true;
}

Ref<AudioEffectInstance> AudioEffectConvolutionReverb::_instantiate() {
	Ref<AudioEffectConvolutionReverbInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectConvolutionReverb>(this);
	return ins;
}

String AudioEffectConvolutionReverb::GetImpulseResponsePath() const {
	return impulseResponsePath;
}

void AudioEffectConvolutionReverb::SetImpulseResponsePath(const String &p_impulse_response_path) {
	if(p_impulse_response_path == impulseResponsePath) {
		return;
	}
	impulseResponsePath = p_impulse_response_path;
	if (p_impulse_response_path == ""){
		impulse = Ref<AudioStreamWAV>();
		update = true;
		return;
	}
	impulse = ResourceLoader::get_singleton()->load(p_impulse_response_path, "wav");
	if (!impulse.is_valid()) {
		update = false;
		impulse = Ref<AudioStreamWAV>();
		print_error("Failed to load impulse response: " + p_impulse_response_path);
		return;
	}
	print_line("Impulse response loaded: " + p_impulse_response_path);
	update = true;
	return;
}

float AudioEffectConvolutionReverb::GetProcessTime() const {
	return time;
}

void AudioEffectConvolutionReverb::SetMix(float p_mix) {
	mix = p_mix;
}

float AudioEffectConvolutionReverb::GetMix() const {
	return mix;
}

void AudioEffectConvolutionReverb::SetUseAVX(bool p_use_avx) {
	useAVX = p_use_avx;
	if (useAVX) {
		complexMulAccumulate = ComplexMultiplyAccumulateAVX;
	}else {
		complexMulAccumulate = ComplexMultiplyAccumulate;
	}
}

bool AudioEffectConvolutionReverb::GetUseAVX() const {
	return useAVX;
}

void AudioEffectConvolutionReverb::SetGain(float p_gain) {
	gain = p_gain;
}

float AudioEffectConvolutionReverb::GetGain() const {
	return gain;
}

void AudioEffectConvolutionReverb::_bind_methods() {
	ClassDB::bind_method(D_METHOD("SetImpulseResponsePath", "path"), &AudioEffectConvolutionReverb::SetImpulseResponsePath);
	ClassDB::bind_method(D_METHOD("GetImpulseResponsePath"), &AudioEffectConvolutionReverb::GetImpulseResponsePath);
	ClassDB::bind_method(D_METHOD("GetProcessTime"), &AudioEffectConvolutionReverb::GetProcessTime);
	ClassDB::bind_method(D_METHOD("SetMix", "mix"), &AudioEffectConvolutionReverb::SetMix);
	ClassDB::bind_method(D_METHOD("GetMix"), &AudioEffectConvolutionReverb::GetMix);
	ClassDB::bind_method(D_METHOD("SetGain", "gain"), &AudioEffectConvolutionReverb::SetGain);
	ClassDB::bind_method(D_METHOD("GetGain"), &AudioEffectConvolutionReverb::GetGain);
	ClassDB::bind_method(D_METHOD("SetUseAVX", "use_avx"), &AudioEffectConvolutionReverb::SetUseAVX);
	ClassDB::bind_method(D_METHOD("GetUseAVX"), &AudioEffectConvolutionReverb::GetUseAVX);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "ImpulsePath", PROPERTY_HINT_FILE, "*.wav"), "SetImpulseResponsePath", "GetImpulseResponsePath");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Mix", PROPERTY_HINT_RANGE, "0,1,0.001"), "SetMix", "GetMix");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "Gain", PROPERTY_HINT_RANGE, "0,2,0.001"), "SetGain", "GetGain");	
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "UseAVX"), "SetUseAVX", "GetUseAVX");

}
