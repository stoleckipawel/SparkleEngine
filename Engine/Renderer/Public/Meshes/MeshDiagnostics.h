#pragma once

#include "../RendererAPI.h"

#include <array>
#include <cstdint>
#include <vector>

enum class MeshDiagnosticsResidencyState : std::uint8_t
{
	Unloaded,
	Resident,
};

struct SPARKLE_RENDERER_API MeshDiagnosticsBounds final
{
	std::array<float, 3> Min = {0.0f, 0.0f, 0.0f};
	std::array<float, 3> Max = {0.0f, 0.0f, 0.0f};
	bool IsValid = false;
};

struct SPARKLE_RENDERER_API MeshDiagnosticsRow final
{
	std::uintptr_t MeshRuntimeId = 0;
	std::uintptr_t GpuMeshRuntimeId = 0;
	std::uint64_t MeshAssetId = 0;
	MeshDiagnosticsResidencyState ResidencyState = MeshDiagnosticsResidencyState::Unloaded;
	std::uint32_t VertexCount = 0;
	std::uint32_t IndexCount = 0;
	std::uint32_t TriangleCount = 0;
	std::uint32_t VertexStrideBytes = 0;
	std::uint32_t IndexStrideBytes = 0;
	std::uint64_t EstimatedCpuByteSize = 0;
	std::uint64_t EstimatedGpuByteSize = 0;
	std::uint32_t InstanceCount = 0;
	std::uint32_t VisibleInstanceCount = 0;
	std::uint32_t FirstMaterialSlot = 0;
	MeshDiagnosticsBounds Bounds;
	bool CpuLoaded = false;
	bool GpuResident = false;
	bool HasMaterial = false;
};

struct SPARKLE_RENDERER_API MeshGeometryInstancingDiagnostics final
{
	std::uint32_t CandidateItemCount = 0;
	std::uint32_t AuthoredBatchCount = 0;
	std::uint32_t PreservedGroupBatchCount = 0;
	std::uint32_t AutoBatchCount = 0;
	std::uint32_t RejectedCandidateCount = 0;
	std::uint32_t RejectedMissingGpuMeshCount = 0;
	std::uint32_t RejectedInvalidMaterialCount = 0;
	std::uint32_t RejectedIncompatibleGroupCount = 0;
	std::uint32_t SingleInstanceBatchCount = 0;
	std::uint32_t RenderableInstanceCount = 0;
	std::uint32_t MeshBatchCount = 0;
	std::uint32_t SubmittedInstanceCount = 0;
	std::uint32_t EstimatedGBufferDrawCallsSaved = 0;
};

struct SPARKLE_RENDERER_API MeshDiagnosticsSnapshot final
{
	std::vector<MeshDiagnosticsRow> Rows;
	MeshGeometryInstancingDiagnostics GeometryInstancing;
};
