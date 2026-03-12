#pragma once

#include <dxgi1_6.h>

#if defined(_DEBUG)
	#define ENGINE_SHADERS_OPTIMIZED 1
	#define ENGINE_SHADERS_DEBUG 1
#endif

#if defined(_DEBUG)
	#define ENGINE_GPU_VALIDATION 1
#endif

#if defined(_DEBUG)
	#define ENGINE_REPORT_LIVE_OBJECTS 1
#endif

namespace RHISettings
{
	inline constexpr unsigned FramesInFlight = 2u;

	inline constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	inline constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	inline bool VSync = true;

	inline bool PreferHighPerformanceAdapter = true;

	inline constexpr int ShaderModelMajor = 6;
	inline constexpr int ShaderModelMinor = 0;
}
