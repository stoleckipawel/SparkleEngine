#pragma once

#include "Frame/Temporal/TemporalFrameState.h"
#include "Providers/RenderProviderModel.h"
#include "ShaderData/RenderViewCameraData.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <string>
#include <vector>

enum class ERayReconstructionMotionVectorUnits : std::uint8_t
{
	Unknown = 0,
	PixelDelta = 1,
};

enum class ERayReconstructionMotionVectorDirection : std::uint8_t
{
	Unknown = 0,
	CurrentMinusPrevious = 1,
};

enum class ERayReconstructionDepthConvention : std::uint8_t
{
	Unknown = 0,
	DeviceDepth = 1,
	ReversedDeviceDepth = 2,
	LinearDepth = 3,
};

struct RayReconstructionMotionVectorConvention final
{
	ERayReconstructionMotionVectorUnits Units = ERayReconstructionMotionVectorUnits::Unknown;
	ERayReconstructionMotionVectorDirection Direction = ERayReconstructionMotionVectorDirection::Unknown;
};

struct RayReconstructionInputContract final
{
	RenderProductHandle NoisyInputColor = {};
	RenderProductHandle OutputColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Exposure = {};
	RenderProductHandle Normals = {};
	RenderProductHandle Roughness = {};
	RenderProductHandle DiffuseAlbedo = {};
	RenderProductHandle SpecularAlbedo = {};
	RenderProductHandle SpecularHitDistance = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	bool ResetRequested = false;
	bool HistoryInvalid = true;
	std::string ResetReason;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
	RayReconstructionMotionVectorConvention MotionVectorConvention = {};
	ERayReconstructionDepthConvention DepthConvention = ERayReconstructionDepthConvention::Unknown;
};

struct RayReconstructionInputContractValidation final
{
	bool Valid = false;
	std::vector<std::string> MissingRequirements;
	std::string Summary;
};

RendererProviderRayReconstructionResourceContract BuildRayReconstructionProviderResourceContract(
    const RayReconstructionInputContract& contract) noexcept;
RayReconstructionInputContractValidation ValidateRayReconstructionInputContract(const RayReconstructionInputContract& contract);

const char* RayReconstructionMotionVectorUnitsToString(ERayReconstructionMotionVectorUnits units) noexcept;
const char* RayReconstructionMotionVectorDirectionToString(ERayReconstructionMotionVectorDirection direction) noexcept;
const char* RayReconstructionDepthConventionToString(ERayReconstructionDepthConvention convention) noexcept;
