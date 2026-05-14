#include "PCH.h"

#include "Panels/UsedMeshesPanel.h"

#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Util/UiUtil.h"

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

namespace
{
	constexpr std::array<const char*, 3> kPreviewModes = {"Wireframe", "Solid", "Wire + Solid"};
	constexpr std::size_t kMaxPreviewTriangles = 20000;
	constexpr float kPi = 3.1415926535f;

	std::string FormatRuntimeId(std::uintptr_t id)
	{
		return std::format("0x{:016X}", static_cast<std::uint64_t>(id));
	}

	std::string FormatAssetId(std::uint64_t id)
	{
		return std::format("0x{:016X}", id);
	}

	struct MeshDisplayMetadata final
	{
		std::string DisplayName;
		std::string SourcePath;
	};

	std::filesystem::path BuildCookedMeshMetadataPath(std::uint64_t meshAssetId)
	{
		std::filesystem::path metadataPath = Paths::CookedMeshAsset(meshAssetId);
		metadataPath += ".meta.json";
		return metadataPath;
	}

	bool TryReadTextFile(const std::filesystem::path& path, std::string& outText)
	{
		std::ifstream input(path, std::ios::binary | std::ios::ate);
		if (!input)
		{
			return false;
		}

		const std::ifstream::pos_type fileSize = input.tellg();
		if (fileSize == std::ifstream::pos_type(-1))
		{
			return false;
		}

		outText.resize(static_cast<std::size_t>(fileSize));
		input.seekg(0, std::ios::beg);
		if (!outText.empty())
		{
			input.read(outText.data(), static_cast<std::streamsize>(outText.size()));
		}
		return static_cast<bool>(input);
	}

	std::optional<MeshDisplayMetadata> LoadCookedMeshMetadata(std::uint64_t meshAssetId)
	{
		std::string metadataText;
		if (!TryReadTextFile(BuildCookedMeshMetadataPath(meshAssetId), metadataText))
		{
			return std::nullopt;
		}

		std::string schema;
		if (!Json::TryReadStringProperty(metadataText, "schema", schema) || schema != "cooked-mesh-metadata-v1")
		{
			return std::nullopt;
		}

		MeshDisplayMetadata metadata;
		Json::TryReadStringProperty(metadataText, "displayName", metadata.DisplayName);
		Json::TryReadStringProperty(metadataText, "source", metadata.SourcePath);
		return metadata;
	}

	const MeshDisplayMetadata* FindMeshDisplayMetadata(const MeshDiagnosticsRow& row)
	{
		if (row.MeshAssetId == 0)
		{
			return nullptr;
		}

		static std::unordered_map<std::uint64_t, std::optional<MeshDisplayMetadata>> metadataCache;
		auto [metadataIt, inserted] = metadataCache.try_emplace(row.MeshAssetId);
		if (inserted)
		{
			metadataIt->second = LoadCookedMeshMetadata(row.MeshAssetId);
		}

		return metadataIt->second ? &*metadataIt->second : nullptr;
	}

	std::string FormatMeshDisplayName(const MeshDiagnosticsRow& row)
	{
		if (const MeshDisplayMetadata* metadata = FindMeshDisplayMetadata(row))
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
		if (const MeshDisplayMetadata* metadata = FindMeshDisplayMetadata(row))
		{
			if (!metadata->SourcePath.empty())
			{
				return metadata->SourcePath;
			}
		}
		return row.MeshAssetId != 0 ? FormatAssetId(row.MeshAssetId) : std::string("unknown");
	}

	void DrawWrappedDisabledText(const std::string& text)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopStyleColor();
	}

	const char* FormatResidency(MeshDiagnosticsResidencyState state) noexcept
	{
		switch (state)
		{
			case MeshDiagnosticsResidencyState::Resident:
				return "Resident";
			case MeshDiagnosticsResidencyState::Unloaded:
			default:
				return "Unloaded";
		}
	}

	std::string FormatBytes(std::uint64_t bytes)
	{
		constexpr double KiB = 1024.0;
		constexpr double MiB = KiB * 1024.0;
		constexpr double GiB = MiB * 1024.0;
		if (bytes >= static_cast<std::uint64_t>(GiB))
		{
			return std::format("{:.2f} GiB", static_cast<double>(bytes) / GiB);
		}
		if (bytes >= static_cast<std::uint64_t>(MiB))
		{
			return std::format("{:.2f} MiB", static_cast<double>(bytes) / MiB);
		}
		if (bytes >= static_cast<std::uint64_t>(KiB))
		{
			return std::format("{:.1f} KiB", static_cast<double>(bytes) / KiB);
		}
		return std::format("{} B", bytes);
	}

	std::string FormatMemorySummary(const MeshDiagnosticsRow& row)
	{
		return std::format("GPU {} / CPU {}", FormatBytes(row.EstimatedGpuByteSize), FormatBytes(row.EstimatedCpuByteSize));
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

	struct PreviewBounds final
	{
		MeshPreviewVertex Min;
		MeshPreviewVertex Max;
		bool IsValid = false;
	};

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

void UsedMeshesPanel::SetDiagnosticsProvider(DiagnosticsProvider provider)
{
	m_diagnosticsProvider = std::move(provider);
}

void UsedMeshesPanel::SetPreviewGeometryProvider(PreviewGeometryProvider provider)
{
	m_previewGeometryProvider = std::move(provider);
	m_previewGeometry = {};
	m_previewGeometryMeshRuntimeId = 0;
}

void UsedMeshesPanel::BuildUI(bool disableInteraction)
{
	if (!m_isOpen)
	{
		return;
	}

	RefreshSnapshot();
	ImGui::SetNextWindowSize(ImVec2(1280.0f, 760.0f), ImGuiCond_FirstUseEver);
	const std::string windowTitle = UiUtil::MakeIconLabel(UiUtil::EditorIcon::StaticMesh, "Mesh Tools") + "##Mesh Tools";
	if (!ImGui::Begin(windowTitle.c_str(), &m_isOpen))
	{
		ImGui::End();
		return;
	}

	DrawToolbar();
	ImGui::Separator();

	const ImGuiTableFlags layoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
	if (ImGui::BeginTable("##MeshInspectorLayout", 2, layoutFlags, ImGui::GetContentRegionAvail()))
	{
		ImGui::TableSetupColumn("Meshes", ImGuiTableColumnFlags_WidthStretch, 0.62f);
		ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.38f);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::BeginChild("##MeshTablePane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
		DrawMeshTable(disableInteraction);
		ImGui::EndChild();
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild(
		    "##MeshPreviewPane",
		    ImVec2(0.0f, 0.0f),
		    ImGuiChildFlags_None,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawSelectedMeshInspector(disableInteraction);
		ImGui::EndChild();
		ImGui::EndTable();
	}

	ImGui::End();
}

void UsedMeshesPanel::RefreshSnapshot()
{
	if (m_diagnosticsProvider)
	{
		m_snapshot = m_diagnosticsProvider();
	}
}

void UsedMeshesPanel::RefreshPreviewGeometry(const MeshDiagnosticsRow& row)
{
	if (m_previewGeometryMeshRuntimeId == row.MeshRuntimeId)
	{
		return;
	}

	m_previewGeometryMeshRuntimeId = row.MeshRuntimeId;
	m_previewGeometry = m_previewGeometryProvider ? m_previewGeometryProvider(row.MeshRuntimeId) : MeshPreviewGeometry{};
}

void UsedMeshesPanel::DrawToolbar()
{
	ImGui::SetNextItemWidth(300.0f);
	const std::string filterHint = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Search, "Filter mesh/source/id/material");
	ImGui::InputTextWithHint("##UsedMeshesFilter", filterHint.c_str(), m_filterBuffer.data(), m_filterBuffer.size());
	ImGui::SameLine();
	ImGui::TextDisabled(
	    "%zu mesh(es), %zu resident, GPU %s, CPU %s estimated",
	    m_snapshot.Rows.size(),
	    CountResidentMeshes(m_snapshot),
	    FormatBytes(SumGpuBytes(m_snapshot)).c_str(),
	    FormatBytes(SumCpuBytes(m_snapshot)).c_str());
}

void UsedMeshesPanel::DrawMeshTable(bool disableInteraction)
{
	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
	                                   ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
	if (!ImGui::BeginTable("##UsedMeshesTable", 9, tableFlags, ImVec2(0.0f, 0.0f)))
	{
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Mesh");
	ImGui::TableSetupColumn("Source");
	ImGui::TableSetupColumn("Uploaded");
	ImGui::TableSetupColumn("Vertices");
	ImGui::TableSetupColumn("Indices");
	ImGui::TableSetupColumn("Triangles");
	ImGui::TableSetupColumn("Memory");
	ImGui::TableSetupColumn("Material");
	ImGui::TableSetupColumn("Bounds");
	ImGui::TableHeadersRow();

	ImGui::BeginDisabled(disableInteraction);
	for (const MeshDiagnosticsRow& row : m_snapshot.Rows)
	{
		if (!MatchesFilter(row))
		{
			continue;
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		const bool selected = row.MeshRuntimeId == m_selectedMeshRuntimeId;
		const std::string displayName = FormatMeshDisplayName(row);
		if (ImGui::Selectable(displayName.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
		{
			m_selectedMeshRuntimeId = row.MeshRuntimeId;
			m_previewGeometryMeshRuntimeId = 0;
			m_previewGeometry = {};
		}
		ImGui::TableNextColumn();
		const std::string sourcePath = FormatMeshSourcePath(row);
		ImGui::TextUnformatted(sourcePath.c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(row.GpuResident ? "yes" : "no");
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<unsigned int>(row.VertexCount));
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<unsigned int>(row.IndexCount));
		ImGui::TableNextColumn();
		ImGui::Text("%u", static_cast<unsigned int>(row.TriangleCount));
		ImGui::TableNextColumn();
		const std::string memory = FormatMemorySummary(row);
		ImGui::TextUnformatted(memory.c_str());
		ImGui::TableNextColumn();
		const std::string material = FormatMaterial(row);
		ImGui::TextUnformatted(material.c_str());
		ImGui::TableNextColumn();
		const std::string bounds = FormatBoundsExtent(row.Bounds);
		ImGui::TextUnformatted(bounds.c_str());
	}
	ImGui::EndDisabled();
	ImGui::EndTable();
}

void UsedMeshesPanel::DrawSelectedMeshInspector(bool disableInteraction)
{
	const MeshDiagnosticsRow* selectedRow = GetSelectedRow();
	if (selectedRow == nullptr)
	{
		ImGui::TextDisabled("Select a mesh to inspect memory, geometry, residency, bounds, and preview.");
		return;
	}

	RefreshPreviewGeometry(*selectedRow);
	const std::string displayName = FormatMeshDisplayName(*selectedRow);
	const std::string sourcePath = FormatMeshSourcePath(*selectedRow);
	ImGui::TextUnformatted(displayName.c_str());
	DrawWrappedDisabledText(sourcePath);
	ImGui::Separator();
	DrawPreviewControls(disableInteraction);
	DrawPreview(*selectedRow);
	ImGui::SeparatorText("Runtime Properties");
	ImGui::BeginChild("##MeshDetailsPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
	DrawSelectedMeshDetails(*selectedRow);
	ImGui::EndChild();
}

void UsedMeshesPanel::DrawPreviewControls(bool disableInteraction)
{
	ImGui::BeginDisabled(disableInteraction);
	ImGui::SetNextItemWidth(150.0f);
	ImGui::Combo("Mode", &m_previewModeIndex, kPreviewModes.data(), static_cast<int>(kPreviewModes.size()));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("Yaw", &m_previewYaw, -kPi, kPi, "%.2f");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("Pitch", &m_previewPitch, -1.45f, 1.45f, "%.2f");
	ImGui::SameLine();
	if (UiUtil::DrawEditorIconButton(UiUtil::EditorIcon::Reset, "##ResetMeshPreview", "Reset preview view"))
	{
		m_previewYaw = 0.65f;
		m_previewPitch = -0.35f;
	}
	ImGui::EndDisabled();
}

void UsedMeshesPanel::DrawPreview(const MeshDiagnosticsRow& row) const
{
	const float detailsReserveHeight = 214.0f;
	ImVec2 availableRegion = ImGui::GetContentRegionAvail();
	availableRegion.y = (std::max) (128.0f, availableRegion.y - detailsReserveHeight);

	ImGui::BeginChild("##MeshPreview", ImVec2(0.0f, availableRegion.y), ImGuiChildFlags_Borders);
	const ImVec2 canvasMin = ImGui::GetCursorScreenPos();
	const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("##MeshPreviewCanvas", canvasSize);
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 canvasMax(canvasMin.x + canvasSize.x, canvasMin.y + canvasSize.y);
	drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(18, 20, 24, 255));
	drawList->AddRect(canvasMin, canvasMax, IM_COL32(68, 74, 84, 255));

	if (!m_previewGeometry.IsValid())
	{
		drawList->AddText(
		    ImVec2(canvasMin.x + 12.0f, canvasMin.y + 12.0f),
		    IM_COL32(160, 166, 176, 255),
		    "Preview geometry is unavailable.");
		ImGui::EndChild();
		return;
	}

	const PreviewBounds bounds = ComputePreviewBounds(m_previewGeometry);
	if (!bounds.IsValid || canvasSize.x <= 1.0f || canvasSize.y <= 1.0f)
	{
		ImGui::EndChild();
		return;
	}

	const MeshPreviewVertex center{
	    (bounds.Min.X + bounds.Max.X) * 0.5f,
	    (bounds.Min.Y + bounds.Max.Y) * 0.5f,
	    (bounds.Min.Z + bounds.Max.Z) * 0.5f};
	float projectedMinX = (std::numeric_limits<float>::max)();
	float projectedMinY = (std::numeric_limits<float>::max)();
	float projectedMaxX = (std::numeric_limits<float>::lowest)();
	float projectedMaxY = (std::numeric_limits<float>::lowest)();
	for (const MeshPreviewVertex& vertex : m_previewGeometry.Vertices)
	{
		const MeshPreviewVertex rotated = RotatePreviewVertex(vertex, center, m_previewYaw, m_previewPitch);
		projectedMinX = (std::min) (projectedMinX, rotated.X);
		projectedMinY = (std::min) (projectedMinY, rotated.Y);
		projectedMaxX = (std::max) (projectedMaxX, rotated.X);
		projectedMaxY = (std::max) (projectedMaxY, rotated.Y);
	}

	const float projectedWidth = (std::max) (projectedMaxX - projectedMinX, 0.001f);
	const float projectedHeight = (std::max) (projectedMaxY - projectedMinY, 0.001f);
	const float scale = 0.86f * (std::min) (canvasSize.x / projectedWidth, canvasSize.y / projectedHeight);
	const ImVec2 canvasCenter(canvasMin.x + (canvasSize.x * 0.5f), canvasMin.y + (canvasSize.y * 0.5f));
	const auto projectVertex = [&](const MeshPreviewVertex& vertex) noexcept
	{
		const MeshPreviewVertex rotated = RotatePreviewVertex(vertex, center, m_previewYaw, m_previewPitch);
		return ImVec2(canvasCenter.x + (rotated.X * scale), canvasCenter.y - (rotated.Y * scale));
	};

	const bool drawSolid = m_previewModeIndex == 1 || m_previewModeIndex == 2;
	const bool drawWire = m_previewModeIndex == 0 || m_previewModeIndex == 2;
	const std::size_t triangleCount = m_previewGeometry.Indices.size() / 3;
	const std::size_t previewTriangleCount = (std::min) (triangleCount, kMaxPreviewTriangles);
	if (drawSolid)
	{
		const ImDrawListFlags previousDrawListFlags = drawList->Flags;
		if (!drawWire)
		{
			drawList->Flags &= ~ImDrawListFlags_AntiAliasedFill;
		}

		const ImU32 solidColor = row.GpuResident
		                         ? (drawWire ? IM_COL32(89, 142, 199, 84) : IM_COL32(89, 142, 199, 230))
		                         : (drawWire ? IM_COL32(128, 128, 128, 72) : IM_COL32(128, 128, 128, 220));
		for (std::size_t triangleIndex = 0; triangleIndex < previewTriangleCount; ++triangleIndex)
		{
			const std::uint32_t index0 = m_previewGeometry.Indices[(triangleIndex * 3) + 0];
			const std::uint32_t index1 = m_previewGeometry.Indices[(triangleIndex * 3) + 1];
			const std::uint32_t index2 = m_previewGeometry.Indices[(triangleIndex * 3) + 2];
			if (index0 >= m_previewGeometry.Vertices.size() || index1 >= m_previewGeometry.Vertices.size() ||
			    index2 >= m_previewGeometry.Vertices.size())
			{
				continue;
			}

			drawList->AddTriangleFilled(
			    projectVertex(m_previewGeometry.Vertices[index0]),
			    projectVertex(m_previewGeometry.Vertices[index1]),
			    projectVertex(m_previewGeometry.Vertices[index2]),
			    solidColor);
		}
		drawList->Flags = previousDrawListFlags;
	}

	if (drawWire)
	{
		const ImU32 wireColor = row.GpuResident ? IM_COL32(176, 212, 255, 220) : IM_COL32(184, 184, 184, 210);
		for (std::size_t triangleIndex = 0; triangleIndex < previewTriangleCount; ++triangleIndex)
		{
			const std::uint32_t index0 = m_previewGeometry.Indices[(triangleIndex * 3) + 0];
			const std::uint32_t index1 = m_previewGeometry.Indices[(triangleIndex * 3) + 1];
			const std::uint32_t index2 = m_previewGeometry.Indices[(triangleIndex * 3) + 2];
			if (index0 >= m_previewGeometry.Vertices.size() || index1 >= m_previewGeometry.Vertices.size() ||
			    index2 >= m_previewGeometry.Vertices.size())
			{
				continue;
			}

			const ImVec2 point0 = projectVertex(m_previewGeometry.Vertices[index0]);
			const ImVec2 point1 = projectVertex(m_previewGeometry.Vertices[index1]);
			const ImVec2 point2 = projectVertex(m_previewGeometry.Vertices[index2]);
			drawList->AddLine(point0, point1, wireColor, 1.0f);
			drawList->AddLine(point1, point2, wireColor, 1.0f);
			drawList->AddLine(point2, point0, wireColor, 1.0f);
		}
	}

	if (triangleCount > previewTriangleCount)
	{
		drawList->AddText(
		    ImVec2(canvasMin.x + 12.0f, canvasMin.y + 12.0f),
		    IM_COL32(192, 196, 204, 255),
		    "Preview truncated for large mesh.");
	}

	ImGui::EndChild();
}

void UsedMeshesPanel::DrawSelectedMeshDetails(const MeshDiagnosticsRow& row) const
{
	const std::string meshRuntimeId = FormatRuntimeId(row.MeshRuntimeId);
	const std::string gpuRuntimeId = row.GpuMeshRuntimeId != 0 ? FormatRuntimeId(row.GpuMeshRuntimeId) : std::string("none");
	const std::string meshAssetId = row.MeshAssetId != 0 ? FormatAssetId(row.MeshAssetId) : std::string("none");
	const std::string vertices = std::to_string(row.VertexCount);
	const std::string indices = std::to_string(row.IndexCount);
	const std::string triangles = std::to_string(row.TriangleCount);
	const std::string cpuBytes = FormatBytes(row.EstimatedCpuByteSize);
	const std::string gpuBytes = FormatBytes(row.EstimatedGpuByteSize);
	const std::string instances = std::to_string(row.InstanceCount);
	const std::string visibleInstances = std::to_string(row.VisibleInstanceCount);
	const std::string vertexStride = std::to_string(row.VertexStrideBytes);
	const std::string indexStride = std::to_string(row.IndexStrideBytes);
	const std::string material = FormatMaterial(row);
	const std::string boundsMin = row.Bounds.IsValid ? FormatBoundsPoint(row.Bounds.Min) : std::string("unknown");
	const std::string boundsMax = row.Bounds.IsValid ? FormatBoundsPoint(row.Bounds.Max) : std::string("unknown");
	const std::string boundsExtent = FormatBoundsExtent(row.Bounds);

	if (const MeshDisplayMetadata* metadata = FindMeshDisplayMetadata(row); metadata != nullptr && !metadata->SourcePath.empty())
	{
		UiUtil::DrawKeyValueRow("Source", metadata->SourcePath.c_str());
	}
	UiUtil::DrawKeyValueRow("CPU Loaded", row.CpuLoaded ? "yes" : "no");
	UiUtil::DrawKeyValueRow("GPU Resident", row.GpuResident ? "yes" : "no");
	UiUtil::DrawKeyValueRow("Residency", FormatResidency(row.ResidencyState));
	UiUtil::DrawKeyValueRow("Vertices", vertices.c_str());
	UiUtil::DrawKeyValueRow("Indices", indices.c_str());
	UiUtil::DrawKeyValueRow("Triangles", triangles.c_str());
	UiUtil::DrawKeyValueRow("CPU Memory", cpuBytes.c_str());
	UiUtil::DrawKeyValueRow("GPU Memory", gpuBytes.c_str());
	UiUtil::DrawKeyValueRow("Vertex Stride", vertexStride.c_str());
	UiUtil::DrawKeyValueRow("Index Stride", indexStride.c_str());
	UiUtil::DrawKeyValueRow("Instances", instances.c_str());
	UiUtil::DrawKeyValueRow("Visible", visibleInstances.c_str());
	UiUtil::DrawKeyValueRow("Material", material.c_str());
	UiUtil::DrawKeyValueRow("Bounds Min", boundsMin.c_str());
	UiUtil::DrawKeyValueRow("Bounds Max", boundsMax.c_str());
	UiUtil::DrawKeyValueRow("Bounds Extent", boundsExtent.c_str());
	UiUtil::DrawKeyValueRow("Mesh Asset ID", meshAssetId.c_str());
	UiUtil::DrawKeyValueRow("Mesh ID", meshRuntimeId.c_str());
	UiUtil::DrawKeyValueRow("GPU Mesh ID", gpuRuntimeId.c_str());
}

const MeshDiagnosticsRow* UsedMeshesPanel::GetSelectedRow() const noexcept
{
	for (const MeshDiagnosticsRow& row : m_snapshot.Rows)
	{
		if (row.MeshRuntimeId == m_selectedMeshRuntimeId)
		{
			return &row;
		}
	}
	return nullptr;
}

bool UsedMeshesPanel::MatchesFilter(const MeshDiagnosticsRow& row) const
{
	const std::string_view filter(m_filterBuffer.data());
	if (filter.empty())
	{
		return true;
	}

	const std::string displayName = FormatMeshDisplayName(row);
	const std::string meshRuntimeId = FormatRuntimeId(row.MeshRuntimeId);
	const std::string gpuRuntimeId = row.GpuMeshRuntimeId != 0 ? FormatRuntimeId(row.GpuMeshRuntimeId) : std::string("none");
	const std::string meshAssetId = row.MeshAssetId != 0 ? FormatAssetId(row.MeshAssetId) : std::string("none");
	const std::string sourcePath = FormatMeshSourcePath(row);
	const std::string residency = FormatResidency(row.ResidencyState);
	const std::string material = FormatMaterial(row);
	const std::string memory = FormatMemorySummary(row);
	return Strings::ContainsIgnoreCase(displayName, filter) || Strings::ContainsIgnoreCase(sourcePath, filter) ||
	       Strings::ContainsIgnoreCase(meshAssetId, filter) ||
	       Strings::ContainsIgnoreCase(meshRuntimeId, filter) ||
	       Strings::ContainsIgnoreCase(gpuRuntimeId, filter) || Strings::ContainsIgnoreCase(residency, filter) ||
	       Strings::ContainsIgnoreCase(material, filter) || Strings::ContainsIgnoreCase(memory, filter);
}
