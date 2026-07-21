#pragma once

#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"
#include "Renderer/Public/Meshes/MeshDiagnostics.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace MeshDiagnosticsPresentation
{
	extern const std::array<const char*, 3> PreviewModes;
	extern const std::size_t MaxPreviewTriangles;
	extern const float Pi;

	struct PreviewBounds final
	{
		MeshPreviewVertex Min;
		MeshPreviewVertex Max;
		bool IsValid = false;
	};

	std::string FormatRuntimeId(std::uintptr_t id);
	std::string FormatAssetId(std::uint64_t id);
	std::string FormatMeshDisplayName(const MeshDiagnosticsRow& row);
	std::string FormatMeshSourcePath(const MeshDiagnosticsRow& row);
	std::optional<std::string> FindAuthoredSourcePath(const MeshDiagnosticsRow& row);
	const char* FormatResidency(MeshDiagnosticsResidencyState state) noexcept;
	std::string FormatMemorySummary(const MeshDiagnosticsRow& row);
	std::string FormatMaterial(const MeshDiagnosticsRow& row);
	std::string FormatBoundsExtent(const MeshDiagnosticsBounds& bounds);
	std::string FormatBoundsPoint(const std::array<float, 3>& point);
	std::size_t CountResidentMeshes(const MeshDiagnosticsSnapshot& snapshot) noexcept;
	std::uint64_t SumCpuBytes(const MeshDiagnosticsSnapshot& snapshot) noexcept;
	std::uint64_t SumGpuBytes(const MeshDiagnosticsSnapshot& snapshot) noexcept;
	PreviewBounds ComputePreviewBounds(const MeshPreviewGeometry& geometry) noexcept;
	MeshPreviewVertex RotatePreviewVertex(
	    const MeshPreviewVertex& vertex,
	    const MeshPreviewVertex& center,
	    float yaw,
	    float pitch) noexcept;
}
