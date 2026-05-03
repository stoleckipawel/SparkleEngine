#pragma once

#include "../Formats/PixelFormat.h"

#include <cstddef>
#include <cstdint>

namespace RenderConfig
{
	inline constexpr unsigned FramesInFlight = 2u;

	inline constexpr PixelFormat BackBufferFormat = PixelFormat::R8G8B8A8_UNorm;
	inline constexpr PixelFormat SceneColorFormat = BackBufferFormat;

	inline constexpr PixelFormat DepthStencilFormat = PixelFormat::D24_UNorm_S8_UInt;

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
		inline constexpr PixelFormat ShadowMapFormat = PixelFormat::R32_Float;
	}  // namespace Shadows

	namespace GBuffer
	{
		inline constexpr PixelFormat BaseColorFormat = PixelFormat::R8G8B8A8_UNorm;
		inline constexpr PixelFormat NormalFormat = PixelFormat::R16G16B16A16_Float;
		inline constexpr PixelFormat MaterialFormat = PixelFormat::R8G8B8A8_UNorm;
		inline constexpr PixelFormat EmissiveFormat = PixelFormat::R16G16B16A16_Float;
		inline constexpr PixelFormat SceneColorFormat = RenderConfig::SceneColorFormat;
	}  // namespace GBuffer
}  // namespace RenderConfig
