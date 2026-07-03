#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

struct SourceImportResult;

class FbxImportDiagnosticLog final
{
  public:
	static void ReportMissingFile(const std::filesystem::path& filePath, SourceImportResult& result);
	static void ReportParseFailure(const std::filesystem::path& filePath, std::string_view errorMessage, SourceImportResult& result);
	static void ReportNoSupportedStaticMeshes(const std::filesystem::path& filePath, SourceImportResult& result);

	static void ReportIgnoredAnimations(std::size_t count, SourceImportResult& result);
	static void ReportIgnoredEmbeddedTextures(std::size_t count, SourceImportResult& result);
	static void ReportIgnoredCameras(std::size_t count, SourceImportResult& result);
	static void ReportIgnoredLights(std::size_t count, SourceImportResult& result);

	static void ReportUnsupportedShadingModel(std::uint32_t materialIndex, int shadingModel, SourceImportResult& result);
	static void ReportMultipleTextures(std::uint32_t materialIndex, std::string_view slotName, std::uint32_t ignoredTextureCount, SourceImportResult& result);
	static void ReportEmbeddedTexture(std::uint32_t materialIndex, std::string_view slotName, std::string_view texturePath, SourceImportResult& result);
	static void ReportInvalidTexturePath(
	    std::uint32_t materialIndex,
	    std::string_view slotName,
	    std::string_view texturePath,
	    SourceImportResult& result);

	static void ReportInvalidMeshIndex(std::string_view nodeName, std::uint32_t meshIndex, SourceImportResult& result);
	static void ReportMissingVertexPositions(std::string_view meshName, std::string_view nodeName, SourceImportResult& result);
	static void ReportStaticBones(std::string_view meshName, SourceImportResult& result);
	static void ReportIgnoredMorphTargets(std::string_view meshName, SourceImportResult& result);
	static void ReportInvalidTriangleGeometry(std::string_view meshName, SourceImportResult& result);
	static void ReportSkippedNonTriangleFace(std::uint32_t faceIndex, std::string_view meshName, SourceImportResult& result);
	static void ReportInvalidMaterialIndex(std::string_view meshName, std::uint32_t materialIndex, SourceImportResult& result);
};
