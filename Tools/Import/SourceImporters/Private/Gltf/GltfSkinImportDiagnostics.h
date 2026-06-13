#pragma once

#include "SourceImportResult.h"

#include <cstddef>

class GltfSkinImportDiagnostics final
{
  public:
	static std::size_t CountSkinnedMeshNodesImportedThroughSkinPath(const SourceImportResult& result);
	static void ReportStaticOnlySkinnedNodes(SourceImportResult& result);
};
