#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"

class GPUMeshCache;
class Mesh;
class SceneMeshes;

class MeshDiagnosticsCollector final
{
  public:
	static MeshDiagnosticsSnapshot Capture(const SceneMeshes& sceneMeshes, const GPUMeshCache* gpuMeshCache);

  private:
	static void PopulateMeshRow(MeshDiagnosticsRow& row, const Mesh& mesh, const GPUMeshCache* gpuMeshCache);
};