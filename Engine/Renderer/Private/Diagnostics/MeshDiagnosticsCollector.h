#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"

class GPUMeshCache;
class Mesh;
class RenderWorld;

class MeshDiagnosticsCollector final
{
  public:
	static MeshDiagnosticsSnapshot Capture(const RenderWorld& world, const GPUMeshCache* gpuMeshCache);
	static MeshPreviewGeometry CapturePreview(const RenderWorld& world, std::uintptr_t meshRuntimeId);

  private:
	static void PopulateMeshRow(MeshDiagnosticsRow& row, const Mesh& mesh, const GPUMeshCache* gpuMeshCache);
};
