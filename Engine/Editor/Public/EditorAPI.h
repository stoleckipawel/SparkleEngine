#pragma once

#ifdef SPARKLE_STATIC
  #define SPARKLE_EDITOR_API
#else
  #ifdef SPARKLE_EDITOR_EXPORTS
	#define SPARKLE_EDITOR_API __declspec(dllexport)
  #else
	#define SPARKLE_EDITOR_API __declspec(dllimport)
  #endif
#endif
