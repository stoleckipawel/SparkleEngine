#pragma once

#include <string_view>

namespace RendererShaderPackages
{
	inline constexpr std::string_view ComputeClear = "ComputeClear";
	inline constexpr std::string_view DirectShadowSignal = "DirectShadowSignal";
	inline constexpr std::string_view DirectLightReservoirTemporal = "DirectLightReservoirTemporal";
	inline constexpr std::string_view DirectLightReservoirSpatial = "DirectLightReservoirSpatial";
	inline constexpr std::string_view DirectLighting = "DirectLighting";
	inline constexpr std::string_view Exposure = "Exposure";
	inline constexpr std::string_view ExposureReduceScene = "ExposureReduceScene";
	inline constexpr std::string_view ExposureReduceTexture = "ExposureReduceTexture";
	inline constexpr std::string_view ExposureDownsampleScene = "ExposureDownsampleScene";
	inline constexpr std::string_view ExposureDownsampleTexture = "ExposureDownsampleTexture";
	inline constexpr std::string_view GBuffer = "GBuffer";
	inline constexpr std::string_view LinearUpscale = "LinearUpscale";
	inline constexpr std::string_view LightingComposite = "LightingComposite";
	inline constexpr std::string_view OutputEncoding = "OutputEncoding";
	inline constexpr std::string_view PathTracedIndirectLighting = "PathTracedIndirectLighting";
	inline constexpr std::string_view PathTracedDirectLighting = "PathTracedDirectLighting";
	inline constexpr std::string_view RaytracedGBuffer = "RaytracedGBuffer";
	inline constexpr std::string_view ReferenceLightingAccumulation = "ReferenceLightingAccumulation";
	inline constexpr std::string_view RestirIndirectTemporal = "RestirIndirectTemporal";
	inline constexpr std::string_view RestirIndirectSpatial = "RestirIndirectSpatial";
	inline constexpr std::string_view RestirIndirectResolve = "RestirIndirectResolve";
	inline constexpr std::string_view SceneDepth = "SceneDepth";
	inline constexpr std::string_view Sky = "Sky";
	inline constexpr std::string_view SkyMotionVector = "SkyMotionVector";
	inline constexpr std::string_view ToneMapping = "ToneMapping";
	inline constexpr std::string_view VisualizeBuffers = "VisualizeBuffers";
}
