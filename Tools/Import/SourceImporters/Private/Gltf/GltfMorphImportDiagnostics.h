#pragma once

struct SourceImportResult;

class GltfMorphImportDiagnostics final
{
  public:
	static void ReportUnsupportedWeightedNodes(SourceImportResult& result);
};
