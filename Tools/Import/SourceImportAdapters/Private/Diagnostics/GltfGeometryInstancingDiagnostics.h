#pragma once

#include "SourceImportDiagnostics.h"

#include <cstddef>

struct cgltf_data;
struct SourceImportResult;

class GltfGeometryInstancingDiagnostics final
{
  public:
	static SourceGeometryInstancingDiagnostics CaptureBaseline(const cgltf_data* data);
	static void RecordImportedPlacements(SourceImportResult& result) noexcept;

  private:
	static std::size_t CountUniqueMeshPrimitiveCandidates(const cgltf_data* data);
	static std::size_t CountAuthoredInstanceGroups(const cgltf_data* data) noexcept;
};