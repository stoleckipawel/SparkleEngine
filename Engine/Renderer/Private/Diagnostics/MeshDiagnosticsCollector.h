#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"

class GpuMeshCache;
class Mesh;
class RenderWorld;

class MeshDiagnosticsCollector final
{
  public:
	static MeshDiagnosticsSnapshot Capture(const RenderWorld& world, const GpuMeshCache* gpuMeshCache);
	static MeshPreviewGeometry CapturePreview(const RenderWorld& world, std::uintptr_t meshRuntimeId);

  private:
	static void CollectRows(
	    const RenderWorld& world,
	    const GpuMeshCache* gpuMeshCache,
	    MeshDiagnosticsSnapshot& snapshot);
	static void SortRows(MeshDiagnosticsSnapshot& snapshot);
	static MeshGeometryInstancingDiagnostics CaptureGeometryInstancing(
	    const RenderWorld& world,
	    const GpuMeshCache* gpuMeshCache);
	static void PopulateMeshRow(MeshDiagnosticsRow& row, const Mesh& mesh, const GpuMeshCache* gpuMeshCache);
};
