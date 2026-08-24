#include "PCH.h"

#include "Panels/Diagnostics/MeshDiagnosticsPresentation.h"

#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Panels/Diagnostics/PanelDiagnosticsFormatting.h"
#include "Panels/Diagnostics/MeshDiagnosticMetadataCatalog.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <imgui.h>

namespace MeshDiagnosticsPresentation
{

	const std::array<const char*, 3> PreviewModes = {"Wireframe", "Solid", "Wire + Solid"};
	const std::size_t MaxPreviewTriangles = 20000;
	const float Pi = 3.1415926535f;

	std::string FormatRuntimeId(std::uintptr_t id)
	{
		return std::format("0x{:016X}", static_cast<std::uint64_t>(id));
	}

	std::string FormatAssetId(std::uint64_t id)
	{
		return std::format("0x{:016X}", id);
	}

	std::string FormatMeshDisplayName(const MeshDiagnosticsRow& row)
	{
		if (const std::optional<MeshDiagnosticMetadata> metadata = FindMeshDiagnosticMetadata(row))
		{
			if (!metadata->DisplayName.empty())
			{
				return metadata->DisplayName;
			}
		}

		return std::format("Mesh {} verts / {} tris", row.VertexCount, row.TriangleCount);
	}

	std::string FormatMeshSourcePath(const MeshDiagnosticsRow& row)
	{
		if (const std::optional<MeshDiagnosticMetadata> metadata = FindMeshDiagnosticMetadata(row))
		{
			if (!metadata->SourcePath.empty())
			{
				return metadata->SourcePath;
			}
		}
		return row.MeshAssetId != 0 ? FormatAssetId(row.MeshAssetId) : std::string("unknown");
	}

	std::optional<std::string> FindAuthoredSourcePath(const MeshDiagnosticsRow& row)
	{
		const std::optional<MeshDiagnosticMetadata> metadata = FindMeshDiagnosticMetadata(row);
		return metadata.has_value() && !metadata->SourcePath.empty() ? std::optional<std::string>(metadata->SourcePath) : std::nullopt;
	}

	std::string FormatMemorySummary(const MeshDiagnosticsRow& row)
	{
		return std::format(
		    "GPU {} / CPU {}",
		    PanelDiagnosticsFormatting::FormatByteSize(row.EstimatedGpuByteSize),
		    PanelDiagnosticsFormatting::FormatByteSize(row.EstimatedCpuByteSize));
	}

	std::string FormatMaterial(const MeshDiagnosticsRow& row)
	{
		return row.HasMaterial ? std::format("slot {}", row.FirstMaterialSlot) : std::string("none");
	}

	std::string FormatBoundsExtent(const MeshDiagnosticsBounds& bounds)
	{
		if (!bounds.IsValid)
		{
			return "unknown";
		}

		return std::format(
		    "{:.2f} x {:.2f} x {:.2f}",
		    bounds.Max[0] - bounds.Min[0],
		    bounds.Max[1] - bounds.Min[1],
		    bounds.Max[2] - bounds.Min[2]);
	}

	std::string FormatBoundsPoint(const std::array<float, 3>& point)
	{
		return std::format("{:.3f}, {:.3f}, {:.3f}", point[0], point[1], point[2]);
	}

	std::size_t CountResidentMeshes(const MeshDiagnosticsSnapshot& snapshot) noexcept
	{
		std::size_t residentCount = 0;
		for (const MeshDiagnosticsRow& row : snapshot.Rows)
		{
			if (row.GpuResident)
			{
				++residentCount;
			}
		}
		return residentCount;
	}

	std::uint64_t SumCpuBytes(const MeshDiagnosticsSnapshot& snapshot) noexcept
	{
		std::uint64_t totalBytes = 0;
		for (const MeshDiagnosticsRow& row : snapshot.Rows)
		{
			totalBytes += row.EstimatedCpuByteSize;
		}
		return totalBytes;
	}

	std::uint64_t SumGpuBytes(const MeshDiagnosticsSnapshot& snapshot) noexcept
	{
		std::uint64_t totalBytes = 0;
		for (const MeshDiagnosticsRow& row : snapshot.Rows)
		{
			totalBytes += row.EstimatedGpuByteSize;
		}
		return totalBytes;
	}

	PreviewBounds ComputePreviewBounds(const MeshPreviewGeometry& geometry) noexcept
	{
		PreviewBounds bounds;
		if (geometry.Vertices.empty())
		{
			return bounds;
		}

		bounds.Min.X = bounds.Min.Y = bounds.Min.Z = (std::numeric_limits<float>::max)();
		bounds.Max.X = bounds.Max.Y = bounds.Max.Z = (std::numeric_limits<float>::lowest)();
		for (const MeshPreviewVertex& vertex : geometry.Vertices)
		{
			bounds.Min.X = (std::min) (bounds.Min.X, vertex.X);
			bounds.Min.Y = (std::min) (bounds.Min.Y, vertex.Y);
			bounds.Min.Z = (std::min) (bounds.Min.Z, vertex.Z);
			bounds.Max.X = (std::max) (bounds.Max.X, vertex.X);
			bounds.Max.Y = (std::max) (bounds.Max.Y, vertex.Y);
			bounds.Max.Z = (std::max) (bounds.Max.Z, vertex.Z);
		}
		bounds.IsValid = true;
		return bounds;
	}

	MeshPreviewVertex RotatePreviewVertex(const MeshPreviewVertex& vertex, const MeshPreviewVertex& center, float yaw, float pitch) noexcept
	{
		const float x = vertex.X - center.X;
		const float y = vertex.Y - center.Y;
		const float z = vertex.Z - center.Z;

		const float cosYaw = std::cos(yaw);
		const float sinYaw = std::sin(yaw);
		const float yawX = (x * cosYaw) + (z * sinYaw);
		const float yawZ = (-x * sinYaw) + (z * cosYaw);

		const float cosPitch = std::cos(pitch);
		const float sinPitch = std::sin(pitch);
		return MeshPreviewVertex{yawX, (y * cosPitch) - (yawZ * sinPitch), (y * sinPitch) + (yawZ * cosPitch)};
	}
}
