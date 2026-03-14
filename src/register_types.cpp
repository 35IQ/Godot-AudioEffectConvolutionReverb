<<<<<<< HEAD
#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "audio_effect_convolution_reverb.h"

using namespace godot;

void initialize_audio_effect_convolution_reverb_module( ModuleInitializationLevel p_level ) {
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}
	GDREGISTER_CLASS( AudioEffectConvolutionReverb );
	GDREGISTER_CLASS( AudioEffectConvolutionReverbInstance );
	
}

void uninitialize_audio_effect_convolution_reverb_module( ModuleInitializationLevel p_level ) {
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
			return;
	}
}

extern "C" {
	// Initialization.
	GDExtensionBool GDE_EXPORT audio_effect_convolution_reverb_module_init( GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization ) {
		godot::GDExtensionBinding::InitObject init_obj( p_get_proc_address, p_library, r_initialization );

		init_obj.register_initializer( initialize_audio_effect_convolution_reverb_module );
		init_obj.register_terminator( uninitialize_audio_effect_convolution_reverb_module );
		init_obj.set_minimum_library_initialization_level( MODULE_INITIALIZATION_LEVEL_SCENE );

		return init_obj.init();
	}

=======
#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "audio_effect_convolution_reverb.h"

using namespace godot;

void initialize_audio_effect_convolution_reverb_module( ModuleInitializationLevel p_level ) {
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}
	GDREGISTER_CLASS( AudioEffectConvolutionReverb );
	GDREGISTER_CLASS( AudioEffectConvolutionReverbInstance );
	
}

void uninitialize_audio_effect_convolution_reverb_module( ModuleInitializationLevel p_level ) {
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
			return;
	}
}

extern "C" {
	// Initialization.
	GDExtensionBool GDE_EXPORT audio_effect_convolution_reverb_module_init( GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization ) {
		godot::GDExtensionBinding::InitObject init_obj( p_get_proc_address, p_library, r_initialization );

		init_obj.register_initializer( initialize_audio_effect_convolution_reverb_module );
		init_obj.register_terminator( uninitialize_audio_effect_convolution_reverb_module );
		init_obj.set_minimum_library_initialization_level( MODULE_INITIALIZATION_LEVEL_SCENE );

		return init_obj.init();
	}

>>>>>>> f4ff7c7e760ef902c0dd87dabebf1cf6bef0a404
}