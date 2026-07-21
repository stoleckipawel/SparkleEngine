#pragma once

#include "Renderer/Public/Meshes/MeshDiagnostics.h"

#include <optional>
#include <string>

struct MeshDiagnosticMetadata final
{
	std::string DisplayName;
	std::string SourcePath;
};

std::optional<MeshDiagnosticMetadata> FindMeshDiagnosticMetadata(const MeshDiagnosticsRow& row);
