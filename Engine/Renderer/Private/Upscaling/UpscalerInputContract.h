#pragma once

#include "Frame/TemporalFrameState.h"
#include "Viewport/ViewportContracts.h"

#include <cstdint>
#include <string>
#include <vector>

enum class EUpscalerMotionVectorUnits : std::uint8_t
{
	Unknown = 0,
	PixelDelta = 1
};

enum class EUpscalerMotionVectorDirection : std::uint8_t
{
	Unknown = 0,
	CurrentMinusPrevious = 1
};

enum class EUpscalerDepthConvention : std::uint8_t
{
	Unknown = 0,
	DeviceDepth = 1,
	ReversedDeviceDepth = 2
};

struct UpscalerMotionVectorConvention final
{
	EUpscalerMotionVectorUnits Units = EUpscalerMotionVectorUnits::Unknown;
	EUpscalerMotionVectorDirection Direction = EUpscalerMotionVectorDirection::Unknown;
};

struct UpscalerInputContract final
{
	RenderProductHandle HudlessSceneColor = {};
	RenderProductHandle Depth = {};
	RenderProductHandle MotionVectors = {};
	RenderProductHandle Exposure = {};
	RenderProductHandle FinalOutput = {};
	RenderViewportExtent RenderExtent = {};
	RenderViewportExtent OutputExtent = {};
	std::uint64_t FrameIndex = 0;
	bool HdrMetadataAvailable = false;
	bool ExposureRequired = false;
	bool ResetRequested = false;
	bool CameraCut = false;
	bool HistoryInvalid = true;
	std::string ResetReason;
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
	UpscalerMotionVectorConvention MotionVectorConvention = {};
	EUpscalerDepthConvention DepthConvention = EUpscalerDepthConvention::Unknown;
};

struct UpscalerInputContractValidation final
{
	bool Valid = false;
	std::vector<std::string> MissingRequirements;
	std::string Summary;
};

UpscalerInputContractValidation ValidateUpscalerInputContract(const UpscalerInputContract& contract);

const char* UpscalerMotionVectorUnitsToString(EUpscalerMotionVectorUnits units) noexcept;
const char* UpscalerMotionVectorDirectionToString(EUpscalerMotionVectorDirection direction) noexcept;
const char* UpscalerDepthConventionToString(EUpscalerDepthConvention convention) noexcept;
