#pragma once

#include <string_view>

namespace RendererShaderPackages
{
	inline constexpr std::string_view ComputeClear = "ComputeClear";
	inline constexpr std::string_view DirectLighting = "DirectLighting";
	inline constexpr std::string_view DirectLightingVulkanAddress = "DirectLightingVulkanAddress";
	inline constexpr std::string_view GBuffer = "GBuffer";
	inline constexpr std::string_view IndirectLighting = "IndirectLighting";
	inline constexpr std::string_view LightingComposite = "LightingComposite";
	inline constexpr std::string_view Sky = "Sky";
	inline constexpr std::string_view VisualizeBuffers = "VisualizeBuffers";
}
