#pragma once

#include <string_view>

namespace RendererShaderPackages
{
	inline constexpr std::string_view ComputeClear = "ComputeClear";
	inline constexpr std::string_view DirectLightingNoRayQuery = "DirectLightingNoRayQuery";
	inline constexpr std::string_view DirectLighting = "DirectLighting";
	inline constexpr std::string_view DirectLightingVulkanAddress = "DirectLightingVulkanAddress";
	inline constexpr std::string_view GBuffer = "GBuffer";
	inline constexpr std::string_view IndirectDiffuse = "IndirectDiffuse";
	inline constexpr std::string_view LightingComposite = "LightingComposite";
	inline constexpr std::string_view IndirectSpecular = "IndirectSpecular";
	inline constexpr std::string_view PresentScene = "PresentScene";
	inline constexpr std::string_view Sky = "Sky";
	inline constexpr std::string_view VisualizeBuffers = "VisualizeBuffers";
}
