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
		inline constexpr std::size_t MaxPointLights = 512;
		inline constexpr std::size_t MaxSpotLights = 512;
	}

	namespace GBuffer
	{
		inline constexpr PixelFormat BaseColorFormat = PixelFormat::R8G8B8A8_UNorm;
		inline constexpr PixelFormat NormalFormat = PixelFormat::R16G16B16A16_Float;
		inline constexpr PixelFormat MaterialFormat = PixelFormat::R8G8B8A8_UNorm;
		inline constexpr PixelFormat EmissiveFormat = PixelFormat::R16G16B16A16_Float;
		inline constexpr PixelFormat SubsurfaceFormat = PixelFormat::R8G8B8A8_UNorm;
		inline constexpr PixelFormat DeviceZFormat = PixelFormat::R32_Float;
		inline constexpr PixelFormat MotionVectorFormat = PixelFormat::R16G16_Float;
		inline constexpr PixelFormat SceneColorFormat = RenderConfig::SceneColorFormat;
	}  // namespace GBuffer
}  // namespace RenderConfig
