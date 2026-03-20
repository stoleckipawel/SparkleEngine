#pragma once

#include <cstddef>

#include <dxgi1_6.h>

namespace RenderConfig
{
	inline constexpr unsigned FramesInFlight = 2u;

	inline constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	inline constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	inline constexpr int ShaderModelMajor = 6;
	inline constexpr int ShaderModelMinor = 0;

	namespace Lights
	{
		inline constexpr std::size_t MaxDirectionalLights = 2;
	}  // namespace Lights
}  // namespace RenderConfig