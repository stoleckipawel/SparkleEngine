#pragma once

#ifdef SPARKLE_RENDERER_EXPORTS
	#define SPARKLE_RENDERER_API __declspec(dllexport)
#else
	#define SPARKLE_RENDERER_API __declspec(dllimport)
#endif

#ifdef SPARKLE_STATIC
	#undef SPARKLE_RENDERER_API
	#define SPARKLE_RENDERER_API
#endif
