#pragma once

#if defined(_WIN32) && defined(BUILD_SHARED_LIBS)
#ifdef window_EXPORTS
#define  WINDOW_EXPORT __declspec( dllexport )
#else
#define  WINDOW_EXPORT __declspec( dllimport )
#endif
#else
#define	WINDOW_EXPORT
#endif