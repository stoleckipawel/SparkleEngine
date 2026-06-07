#pragma once

struct SourceImportResult;

class GltfImportFeatureDiagnostics final
{
  public:
	static void RecordImportedFeatureSupport(SourceImportResult& result);
};
