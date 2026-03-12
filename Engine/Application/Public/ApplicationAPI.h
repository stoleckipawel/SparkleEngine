#pragma once

#ifdef SPARKLE_STATIC
	#define SPARKLE_APPLICATION_API
#else
	#ifdef SPARKLE_APPLICATION_EXPORTS
		#define SPARKLE_APPLICATION_API __declspec(dllexport)
	#else
		#define SPARKLE_APPLICATION_API __declspec(dllimport)
	#endif
#endif
