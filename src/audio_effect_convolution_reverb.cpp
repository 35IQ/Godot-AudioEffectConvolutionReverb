#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include "audio_effect_convolution_reverb.h"
#include <functional>
#include <immintrin.h>

using namespace godot;


void AudioEffectConvolutionReverbInstance::_process( const AudioFrame *pSrcFrames, AudioFrame *pDstFrames, int pFrameCount ){
	int T = Time::get_singleton()->get_ticks_usec();
	if( pFrameCount == 0 ) {
		return;
	}
	if ( base->update || pFrameCount != blockSize ) {
		base->update = false;
		SetImpulseResponse( pFrameCount );
		print_line( "Impulse response updated." );
	}
	//bypass

	if ( partitionCount == 0 ) {	
		std::memcpy( pDstFrames, pSrcFrames, pFrameCount * sizeof( AudioFrame ) );
		return;	
	}
	
	auto sigL = input[inputPos].left.data();
	auto sigR = input[inputPos].right.data();
	int i;
	for (i = 0; i < blockSize; i++) {
		sigL[i * 2 + 0] = pSrcFrames[i].left;
		sigL[i * 2 + 1] = 0.0f;
		sigR[i * 2 + 0] = pSrcFrames[i].right;
		sigR[i * 2 + 1] = 0.0f;
	}
	// zero padding
	std::memset( sigL + complexLength, 0, ( fftSize - complexLength ) * sizeof( float )  );
	std::memset( sigR + complexLength, 0, ( fftSize - complexLength ) * sizeof( float )  );
	fft.FFT( sigL,  -1 );
	fft.FFT( sigR,  -1 );

	auto impL = impulseFD[0].left.data();
	auto impR = impulseFD[0].right.data();
	auto accL = accumulators[currentAccum].left.data();
	auto accR = accumulators[currentAccum].right.data();

	ComplexMultiplyFirstPartition( sigL, impL, accL, fftSize );
	ComplexMultiplyFirstPartition( sigR, impR, accR, fftSize );

	// convolution sum
	for ( int p = 1; p < partitionCount; p++ ) {
		int idx = inputPos - p;
		idx += ( idx >> 31 ) & partitionCount;

		sigL = input[idx].left.data();
		sigR = input[idx].right.data();
		impL = impulseFD[p].left.data();
		impR = impulseFD[p].right.data();
		ComplexMultiplyAccumulate( sigL, impL, accL, fftSize );
		ComplexMultiplyAccumulate( sigR, impR, accR, fftSize );
	}

	fft.FFT( accL,  1 );
	fft.FFT( accR,  1 );

	// overlap-add
	int prevAccum = currentAccum ^ 1;
	auto prevAccL = accumulators[prevAccum].left.data();
	auto prevAccR = accumulators[prevAccum].right.data();
	for ( i = 0; i < blockSize; i++ ) {
		const int idx = i*2;	
		const int idx2 = idx + complexLength;
		const float outL = ( accL[idx] + prevAccL[idx2] ) * invFFT * base->gain;
		const float outR = ( accR[idx] + prevAccR[idx2] ) * invFFT * base->gain;
		pDstFrames[i].left  = pSrcFrames[i].left  * base->dry + outL * base->wet;
		pDstFrames[i].right = pSrcFrames[i].right * base->dry + outR * base->wet;
	}

	currentAccum = prevAccum;
	inputPos++;
	if (inputPos >= partitionCount)	inputPos = 0;
	base->time = float( Time::get_singleton()->get_ticks_usec() - T ) * 0.000001;
	return;
}

bool AudioEffectConvolutionReverbInstance::SetImpulseResponse( const int pFrameCount ){
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
	if ( base->impulse.ptr()->get_format() == AudioStreamWAV::FORMAT_8_BITS ) {
		for (int i = 0; i < pcmSize ; i += 2){
			impulseLeft.push_back ( float( pcm.decode_s8( i ) 	 ) / 128.0f );
			impulseRight.push_back( float( pcm.decode_s8( i + 1 ) ) / 128.0f );
		}
	} else if(base->impulse.ptr()->get_format() == AudioStreamWAV::FORMAT_16_BITS){
		for (int i = 0; i < pcmSize ; i += 4){
			impulseLeft.push_back ( float( pcm.decode_s16( i ) 	  ) / 32768.0f );
			impulseRight.push_back( float( pcm.decode_s16( i + 2 ) ) / 32768.0f );
		}
	} else {
		print_error("Unsupported audio format for impulse response. Only 8-bit and 16-bit PCM WAV files are supported.");
		return false;
	}
	
	blockSize = pFrameCount;
	complexLength = blockSize << 1;
	fftSize = complexLength << 1;
	fft.SetUpFFT( complexLength );
	invFFT = 1.0f / static_cast<float>( fftSize );
	
	int impulseSize = impulseLeft.size();
	partitionCount = ( impulseSize + blockSize - 1 ) / blockSize;
	print_line( "Impulse samples: " + String::num( impulseSize ) );
	print_line( "Partitions: " + String::num( partitionCount ) );

	impulseFD.clear();
	input.clear();

	impulseFD.resize( partitionCount );
	input.resize( partitionCount );
	
	for ( int p = 0; p < partitionCount; p++ ) {
		input[p].left.resize( fftSize, 0.0f );
		input[p].right.resize( fftSize, 0.0f );
		impulseFD[p].left.resize( fftSize, 0.0f );
		impulseFD[p].right.resize( fftSize, 0.0f );

		for( int i = 0; i < blockSize; i++ ) {
			int idx = p * blockSize + i;
			if ( idx < impulseSize ) {
				impulseFD[p].left[i*2] = impulseLeft[idx];
				impulseFD[p].right[i*2] = impulseRight[idx];
			}
		}

		fft.FFT( impulseFD[p].left.data(),   -1 );
		fft.FFT( impulseFD[p].right.data(),  -1 );
	}
	for( int i = 0; i < 2; i++ ) {
		accumulators[i].left.resize( fftSize, 0.0f );
		accumulators[i].right.resize( fftSize, 0.0f );
	}
	inputPos = 0;
	base->SetMix( base->mix );
	return true;
}

void AudioEffectConvolutionReverbInstance::ComplexMultiplyFirstPartition( const float*__restrict pSignal, const float*__restrict pImpulse, float*__restrict pAccum, int pSize ){
	int simdEnd = ( pSize / 8 ) * 8;
    for ( int i = 0; i < simdEnd; i += 8 ) {
		__m256 x = _mm256_load_ps( pSignal + i );
		__m256 h = _mm256_load_ps( pImpulse + i );
		__m256 xr = _mm256_moveldup_ps( x );
		__m256 xi = _mm256_movehdup_ps( x );
		__m256 h_sw = _mm256_shuffle_ps( h, h, 0xB1 );
		__m256 t1 = _mm256_mul_ps( xr, h ); 
		__m256 t2 = _mm256_mul_ps( xi, h_sw );
		__m256 result = _mm256_addsub_ps( t1, t2 );
		_mm256_store_ps( pAccum + i, result );	
	}
	for ( int i = simdEnd; i < pSize; i += 2 ){
        float r =  pSignal[i] * pImpulse[i] - pSignal[i + 1] * pImpulse[i + 1];
        float im = pSignal[i] * pImpulse[i + 1] + pSignal[i + 1] * pImpulse[i];
        pAccum[i] = r;
        pAccum[i+1] = im;
    }
}

void AudioEffectConvolutionReverbInstance::ComplexMultiplyAccumulate( const float*__restrict pSignal, const float*__restrict pImpulse, float*__restrict pAccum, int pSize ){
	int simdEnd = ( pSize / 8 ) * 8;
    for ( int i = 0; i < simdEnd; i += 8 ) {
		__m256 x = _mm256_load_ps( pSignal + i );
		__m256 h = _mm256_load_ps( pImpulse + i );
		__m256 xr = _mm256_moveldup_ps( x );
		__m256 xi = _mm256_movehdup_ps( x );
		__m256 h_sw = _mm256_shuffle_ps( h, h, 0xB1 );
		__m256 t1 = _mm256_mul_ps( xr, h ); 
		__m256 t2 = _mm256_mul_ps( xi, h_sw );
		__m256 result = _mm256_addsub_ps( t1, t2 );
		__m256 acc = _mm256_load_ps( pAccum + i );
		acc = _mm256_add_ps( acc, result );
		_mm256_store_ps( pAccum + i, acc );
	}
	for ( int i = simdEnd; i < pSize; i += 2 ){
        float r = pSignal[i] * pImpulse[i] - pSignal[i + 1]*pImpulse[i + 1];
        float im = pSignal[i] * pImpulse[i + 1] + pSignal[i + 1]*pImpulse[i];
        pAccum[i] += r;
        pAccum[i + 1] += im;
    }	
}

Ref<AudioEffectInstance> AudioEffectConvolutionReverb::_instantiate() {
	Ref<AudioEffectConvolutionReverbInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectConvolutionReverb>(this);
	return ins;
}

void AudioEffectConvolutionReverb::SetImpulseResponsePath( const String &pImpulseResponsePath ) {
	if( pImpulseResponsePath == impulseResponsePath ) { 
		return;
	}
	
	impulseResponsePath = pImpulseResponsePath;
	if ( pImpulseResponsePath == "" ){
		impulse = Ref<AudioStreamWAV>();
		update = true;
		return;
	}

	impulse = ResourceLoader::get_singleton()->load( pImpulseResponsePath, "wav" );
	if ( !impulse.is_valid() ) {
		update = false;
		impulse = Ref<AudioStreamWAV>();
		print_error( "Failed to load impulse response: " + pImpulseResponsePath );
		return;
	}

	print_line( "Impulse response loaded: " + pImpulseResponsePath );
	update = true;
	return;
}

void AudioEffectConvolutionReverb::_bind_methods() {
	ClassDB::bind_method( D_METHOD( "GetImpulseResponsePath" ),			&AudioEffectConvolutionReverb::GetImpulseResponsePath );
	ClassDB::bind_method( D_METHOD( "SetImpulseResponsePath", "path" ), &AudioEffectConvolutionReverb::SetImpulseResponsePath );
	 
	ClassDB::bind_method( D_METHOD( "GetMix" ), 		 &AudioEffectConvolutionReverb::GetMix );
	ClassDB::bind_method( D_METHOD( "SetMix", "mix" ),   &AudioEffectConvolutionReverb::SetMix );
 
	ClassDB::bind_method( D_METHOD( "SetGain", "gain" ), &AudioEffectConvolutionReverb::SetGain );
	ClassDB::bind_method( D_METHOD( "GetGain" ), 		 &AudioEffectConvolutionReverb::GetGain );
 
 	ClassDB::bind_method( D_METHOD( "GetProcessTime" ),  &AudioEffectConvolutionReverb::GetProcessTime );

	ADD_PROPERTY( PropertyInfo( Variant::STRING, "ImpulsePath", PROPERTY_HINT_FILE, "*.wav" ), "SetImpulseResponsePath", "GetImpulseResponsePath" );
	ADD_PROPERTY( PropertyInfo( Variant::FLOAT, "Mix", PROPERTY_HINT_RANGE, "0,1,0.001" ), "SetMix", "GetMix" );
	ADD_PROPERTY( PropertyInfo( Variant::FLOAT, "Gain", PROPERTY_HINT_RANGE, "0,2,0.001" ), "SetGain", "GetGain" );	
}
