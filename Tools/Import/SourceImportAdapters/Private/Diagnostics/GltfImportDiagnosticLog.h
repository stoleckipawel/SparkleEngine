#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

struct SourceImportResult;

class GltfImportDiagnosticLog final
{
  public:
	static void ReportMissingFile(const std::filesystem::path& filePath, SourceImportResult& result);
	static void ReportParseFailure(std::string_view path, int errorCode, SourceImportResult& result);
	static void ReportBufferLoadFailure(std::string_view path, int errorCode, SourceImportResult& result);
	static void ReportValidationWarning(std::string_view path, int errorCode, SourceImportResult& result);
	static void ReportNoSupportedMeshPrimitives(const std::filesystem::path& filePath, SourceImportResult& result);
	static void ReportLoadedScene(const std::filesystem::path& filePath, const SourceImportResult& result);

	static void ReportIgnoredAnimations(std::size_t count, SourceImportResult& result);
	static void ReportIgnoredMaterialVariants(std::size_t count, SourceImportResult& result);
	static void ReportUnsupportedPointLights(std::size_t count, SourceImportResult& result);
	static void ReportUnsupportedSpotLights(std::size_t count, SourceImportResult& result);
	static void ReportStaticSkinnedNodes(std::size_t count, SourceImportResult& result);
	static void ReportIgnoredWeightedNodes(std::size_t count, SourceImportResult& result);
	static void ReportFlattenedGpuInstancingNodes(std::size_t count, SourceImportResult& result);

	static void ReportSkippedNonTrianglePrimitive(std::string_view primitiveLabel, SourceImportResult& result);
	static void ReportIgnoredMorphTargets(std::string_view primitiveLabel, SourceImportResult& result);
	static void ReportSkippedDracoPrimitive(std::string_view primitiveLabel, SourceImportResult& result);
	static void ReportIgnoredMaterialVariantMappings(std::string_view primitiveLabel, SourceImportResult& result);
	static void ReportSkippedIncompletePrimitive(std::string_view primitiveLabel, SourceImportResult& result);
	static void ReportInvalidMaterialIndex(std::string_view primitiveLabel, std::uint32_t materialIndex, SourceImportResult& result);
	static void ReportMalformedGpuInstancing(std::string_view nodeLabel, std::string_view reason, SourceImportResult& result);

	static void ReportInvalidTexturePath(
	    std::uint32_t materialIndex,
	    std::string_view slotName,
	    std::string_view texturePath,
	    SourceImportResult& result);
	static void ReportEmbeddedTexture(std::uint32_t materialIndex, std::string_view slotName, SourceImportResult& result);
	static void ReportUnsupportedEncodedTextureSources(std::uint32_t materialIndex, std::string_view slotName, SourceImportResult& result);
	static void ReportUnsupportedMaterialFeatures(
	    std::uint32_t materialIndex,
	    std::string_view unsupportedFeatures,
	    SourceImportResult& result);
};
