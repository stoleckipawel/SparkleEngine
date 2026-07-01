#pragma once

#include <string_view>

namespace RendererShaderPackages
{
	inline constexpr std::string_view ComputeClear = "ComputeClear";
	inline constexpr std::string_view DirectLightingNoRayQuery = "DirectLightingNoRayQuery";
	inline constexpr std::string_view DirectLighting = "DirectLighting";
	inline constexpr std::string_view DirectLightingDeviceAddress = "DirectLightingDeviceAddress";
	inline constexpr std::string_view Exposure = "Exposure";
	inline constexpr std::string_view ExposureReduceScene = "ExposureReduceScene";
	inline constexpr std::string_view ExposureReduceTexture = "ExposureReduceTexture";
	inline constexpr std::string_view ExposureDownsampleScene = "ExposureDownsampleScene";
	inline constexpr std::string_view ExposureDownsampleTexture = "ExposureDownsampleTexture";
	inline constexpr std::string_view GBuffer = "GBuffer";
	inline constexpr std::string_view IndirectDiffuse = "IndirectDiffuse";
	inline constexpr std::string_view LightingComposite = "LightingComposite";
	inline constexpr std::string_view IndirectSpecular = "IndirectSpecular";
	inline constexpr std::string_view OutputEncoding = "OutputEncoding";
	inline constexpr std::string_view ReferencePathTracing = "ReferencePathTracing";
	inline constexpr std::string_view Sky = "Sky";
	inline constexpr std::string_view ToneMapping = "ToneMapping";
	inline constexpr std::string_view VisualizeBuffers = "VisualizeBuffers";
}
