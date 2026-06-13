#include "PCH.h"

#include "Diagnostics/FbxImportDiagnosticLog.h"

#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "SourceImportResult.h"

#include <format>

static const auto g_fbxImportDiagnosticLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Fbx");

void FbxImportDiagnosticLog::ReportMissingFile(const std::filesystem::path& filePath, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: File not found: {}", filePath.string()));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void FbxImportDiagnosticLog::ReportParseFailure(const std::filesystem::path& filePath, std::string_view errorMessage, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Failed to parse '{}' ({})", filePath.string(), errorMessage));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void FbxImportDiagnosticLog::ReportNoSupportedStaticMeshes(const std::filesystem::path& filePath, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: No supported static meshes found in '{}'", filePath.string()));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void FbxImportDiagnosticLog::ReportLoadedScene(const std::filesystem::path& filePath, const SourceImportResult& result)
{
	SPDLOG_LOGGER_INFO(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format(
	        "FbxImporter: Loaded '{}' - {} mesh primitives, {} mesh instances, {} materials, textures={}, warnings={}, instancing uniquePrimitiveCandidates={}, placements={}, authoredGroups={}",
	        filePath.filename().string(),
	        result.scene.meshPrimitives.size(),
	        result.scene.meshInstances.size(),
	        result.scene.materials.size(),
	        result.diagnostics.textures.resolvedTextureBindingCount,
	        result.diagnostics.issues.warningMessageCount,
	        result.diagnostics.geometryInstancing.uniqueMeshPrimitiveCandidateCount,
	        result.diagnostics.geometryInstancing.meshPlacementCount,
	        result.diagnostics.geometryInstancing.authoredInstanceGroupCount));
}

void FbxImportDiagnosticLog::ReportIgnoredAnimations(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: {} animations are present and will be ignored", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportIgnoredEmbeddedTextures(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: {} embedded textures are present and will be ignored", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportIgnoredCameras(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: {} cameras are present and will be ignored", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportIgnoredLights(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: {} lights are present and will be ignored", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportUnsupportedShadingModel(std::uint32_t materialIndex, int shadingModel, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Material handle {} uses unsupported shading model {} and will be approximated with Sparkle PBR defaults", materialIndex, shadingModel));
	SourceImportDiagnosticsRecorder::RecordUnsupportedMaterialFeature(result);
}

void FbxImportDiagnosticLog::ReportMultipleTextures(std::uint32_t materialIndex, std::string_view slotName, std::uint32_t ignoredTextureCount, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Material handle {} has multiple {} textures and only the first will be used", materialIndex, slotName));
	SourceImportDiagnosticsRecorder::RecordDuplicateTextureBindings(result, ignoredTextureCount);
}

void FbxImportDiagnosticLog::ReportEmbeddedTexture(
    std::uint32_t materialIndex,
    std::string_view slotName,
    std::string_view texturePath,
    SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Material handle {} uses embedded {} texture '{}' which is not supported yet", materialIndex, slotName, texturePath));
	SourceImportDiagnosticsRecorder::RecordEmbeddedTextureBinding(result);
}

void FbxImportDiagnosticLog::ReportInvalidTexturePath(
    std::uint32_t materialIndex,
    std::string_view slotName,
    std::string_view texturePath,
    SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored", materialIndex, slotName, texturePath));
	SourceImportDiagnosticsRecorder::RecordInvalidTexturePath(result);
}

void FbxImportDiagnosticLog::ReportInvalidMeshIndex(std::string_view nodeName, std::uint32_t meshIndex, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: Node '{}' references invalid mesh index {}", nodeName, meshIndex));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportMissingVertexPositions(std::string_view meshName, std::string_view nodeName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: Skipping mesh '{}' on node '{}' because it has no vertex positions", meshName, nodeName));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportStaticBones(std::string_view meshName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: Mesh '{}' contains bones and will be imported as static geometry only", meshName));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportIgnoredMorphTargets(std::string_view meshName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: Mesh '{}' contains morph targets which will be ignored", meshName));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportInvalidTriangleGeometry(std::string_view meshName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: Mesh '{}' did not produce valid triangle geometry", meshName));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportSkippedNonTriangleFace(std::uint32_t faceIndex, std::string_view meshName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_fbxImportDiagnosticLogger, "{}", std::format("FbxImporter: Skipping non-triangle face {} in mesh '{}'", faceIndex, meshName));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void FbxImportDiagnosticLog::ReportInvalidMaterialIndex(std::string_view meshName, std::uint32_t materialIndex, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_fbxImportDiagnosticLogger,
	    "{}",
	    std::format("FbxImporter: '{}' references invalid material index {} and will use the default material", meshName, materialIndex));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}
