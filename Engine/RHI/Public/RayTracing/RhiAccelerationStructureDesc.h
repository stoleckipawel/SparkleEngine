#pragma once

#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceHandles.h"

#include <cstdint>

struct RhiAccelerationStructureCapabilities
{
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
	bool SupportsAccelerationStructureShaderBinding = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
	std::uint32_t ShaderGroupHandleSizeInBytes = 0;
	std::uint32_t ShaderTableAlignmentInBytes = 0;
	std::uint32_t ShaderTableRecordAlignmentInBytes = 0;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
};

enum class ERhiRayTracingAccelerationStructureType : std::uint8_t
{
	BottomLevel,
	TopLevel,
};

struct RhiRayTracingBufferBinding
{
	RhiResourceHandle Resource = {};
	std::uint64_t OffsetInBytes = 0;
};

struct RhiRayTracingGeometryDesc
{
	RhiRayTracingBufferBinding VertexBuffer = {};
	std::uint32_t VertexStrideInBytes = 0;
	std::uint32_t VertexCount = 0;
	RhiRayTracingBufferBinding IndexBuffer = {};
	std::uint32_t IndexCount = 0;
	RhiIndexFormat IndexFormat = RhiIndexFormat::UInt32;
	bool Opaque = true;
};

struct RhiRayTracingAccelerationStructurePrebuildInfo
{
	std::uint64_t ResultDataMaxSizeInBytes = 0;
	std::uint64_t ScratchDataSizeInBytes = 0;
	std::uint64_t UpdateScratchDataSizeInBytes = 0;
};
