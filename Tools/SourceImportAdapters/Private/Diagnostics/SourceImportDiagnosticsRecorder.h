#pragma once

#include <cstddef>

struct SourceGeometryInstancingDiagnostics;
struct SourceSceneFeatureDiagnostics;
struct SourceImportResult;

class SourceImportDiagnosticsRecorder final
{
  public:
	static void RecordSourceSummary(SourceImportResult& result, std::size_t sourceMeshCount, std::size_t sourceMaterialCount) noexcept;
	static void RecordSceneFeatures(SourceImportResult& result, const SourceSceneFeatureDiagnostics& diagnostics) noexcept;
	static void RecordImportedScenePayload(SourceImportResult& result) noexcept;
	static void RecordGeometryInstancingBaseline(
	    SourceImportResult& result,
	    const SourceGeometryInstancingDiagnostics& diagnostics) noexcept;
	static void RecordGeometryInstancingPrimitiveCandidates(SourceImportResult& result, std::size_t candidateCount) noexcept;
	static void RecordGeometryInstancingPlacements(SourceImportResult& result) noexcept;
	static void RecordWarning(SourceImportResult& result) noexcept;
	static void RecordError(SourceImportResult& result) noexcept;
	static void RecordReferencedTextureBindings(SourceImportResult& result, std::size_t count = 1) noexcept;
	static void RecordDuplicateTextureBindings(SourceImportResult& result, std::size_t count) noexcept;
	static void RecordInvalidTexturePath(SourceImportResult& result) noexcept;
	static void RecordEmbeddedTextureBinding(SourceImportResult& result) noexcept;
	static void RecordUnsupportedTextureBinding(SourceImportResult& result) noexcept;
	static void RecordUnsupportedMaterialFeature(SourceImportResult& result) noexcept;
};