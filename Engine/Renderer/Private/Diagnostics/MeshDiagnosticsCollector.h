#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"

class GpuMeshCache;
class Mesh;
class RenderScene;

class MeshDiagnosticsCollector final
{
public:
	static MeshDiagnosticsSnapshot Capture(const RenderScene& scene, const GpuMeshCache* gpuMeshCache);
	static MeshPreviewGeometry CapturePreview(const RenderScene& scene, std::uintptr_t meshRuntimeId);

private:
	static void CollectRows(const RenderScene& scene, const GpuMeshCache* gpuMeshCache, MeshDiagnosticsSnapshot& snapshot);
	static void SortRows(MeshDiagnosticsSnapshot& snapshot);
	static MeshGeometryInstancingDiagnostics CaptureGeometryInstancing(const RenderScene& scene, const GpuMeshCache* gpuMeshCache);
	static void PopulateMeshRow(MeshDiagnosticsRow& row, const Mesh& mesh, const GpuMeshCache* gpuMeshCache);
};
