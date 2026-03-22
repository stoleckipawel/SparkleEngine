#pragma once

#include <cstddef>
#include <cstdint>

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
	}  

	namespace Shadows
	{
		inline constexpr std::size_t MaxCascades = 2;
		inline constexpr std::size_t MaxShadowMaps = Lights::MaxDirectionalLights * MaxCascades;
		inline constexpr std::uint32_t ShadowMapResolution = 2048;
		inline constexpr float DepthBias = 0.00045f;
		inline constexpr float NormalBias = 0.00125f;
		inline constexpr float ShadowDistance = 60.0f;
		inline constexpr float NearCascadeFraction = 0.2f;
		inline constexpr float LightPadding = 20.0f;
		inline constexpr DXGI_FORMAT ShadowMapFormat = DXGI_FORMAT_R32_FLOAT;
	} 
}  