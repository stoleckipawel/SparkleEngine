#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class UsedMeshesPanel final
{
  public:
	using DiagnosticsProvider = std::function<MeshDiagnosticsSnapshot()>;
	using PreviewGeometryProvider = std::function<MeshPreviewGeometry(std::uintptr_t)>;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetDiagnosticsProvider(DiagnosticsProvider provider);
	void SetPreviewGeometryProvider(PreviewGeometryProvider provider);
	void BuildUI(bool disableInteraction);

  private:
	void RefreshSnapshot();
	void RefreshPreviewGeometry(const MeshDiagnosticsRow& row);
	void DrawToolbar();
	void DrawMeshTable(bool disableInteraction);
	void DrawSelectedMeshInspector(bool disableInteraction);
	void DrawPreviewControls(bool disableInteraction);
	void DrawPreview(const MeshDiagnosticsRow& row) const;
	void DrawSelectedMeshDetails(const MeshDiagnosticsRow& row) const;
	const MeshDiagnosticsRow* GetSelectedRow() const noexcept;
	bool MatchesFilter(const MeshDiagnosticsRow& row) const;

	DiagnosticsProvider m_diagnosticsProvider;
	PreviewGeometryProvider m_previewGeometryProvider;
	MeshDiagnosticsSnapshot m_snapshot;
	MeshPreviewGeometry m_previewGeometry;
	std::array<char, 160> m_filterBuffer{};
	std::uintptr_t m_selectedMeshRuntimeId = 0;
	std::uintptr_t m_previewGeometryMeshRuntimeId = 0;
	float m_previewYaw = 0.65f;
	float m_previewPitch = -0.35f;
	int m_previewModeIndex = 2;
	bool m_isOpen = false;
};
