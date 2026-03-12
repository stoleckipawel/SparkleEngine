#pragma once

#ifdef SPARKLE_STATIC
	#define SPARKLE_ENGINE_API
#else
	#ifdef SPARKLE_ENGINE_EXPORTS
		#define SPARKLE_ENGINE_API __declspec(dllexport)
	#else
		#define SPARKLE_ENGINE_API __declspec(dllimport)
	#endif
#endif

#define SPARKLE_GAMEFRAMEWORK_API SPARKLE_ENGINE_API
