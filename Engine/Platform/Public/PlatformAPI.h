#pragma once

#ifdef SPARKLE_PLATFORM_EXPORTS
	#define SPARKLE_PLATFORM_API __declspec(dllexport)
#else
	#define SPARKLE_PLATFORM_API __declspec(dllimport)
#endif

#ifdef SPARKLE_STATIC
	#undef SPARKLE_PLATFORM_API
	#define SPARKLE_PLATFORM_API
#endif
