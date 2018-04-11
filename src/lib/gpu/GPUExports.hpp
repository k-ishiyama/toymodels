#pragma once

#if defined(_WIN32) && defined(BUILD_SHARED_LIBS)
	#ifdef gpu_EXPORTS
		#define  GPU_EXPORT __declspec( dllexport )
	#else
		#define  GPU_EXPORT __declspec( dllimport )
	#endif
#else
	#define	GPU_EXPORT
#endif