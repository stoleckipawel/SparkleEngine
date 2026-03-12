#pragma once

#ifdef SPARKLE_STATIC
	#define SPARKLE_UI_API
#else
	#ifdef SPARKLE_UI_EXPORTS
		#define SPARKLE_UI_API __declspec(dllexport)
	#else
		#define SPARKLE_UI_API __declspec(dllimport)
	#endif
#endif
