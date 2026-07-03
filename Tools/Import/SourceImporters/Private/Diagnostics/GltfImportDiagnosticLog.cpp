#include "PCH.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include "Diagnostics/SourceImportDiagnosticsRecorder.h"
#include "SourceImportResult.h"

#include <format>

static const auto g_gltfImportDiagnosticLogger = Logging::GetOrCreateLogger("Tools.SourceImporters.Gltf");

void GltfImportDiagnosticLog::ReportMissingFile(const std::filesystem::path& filePath, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(g_gltfImportDiagnosticLogger, "{}", std::format("GltfImporter: File not found: {}", filePath.string()));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void GltfImportDiagnosticLog::ReportParseFailure(std::string_view path, int errorCode, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Failed to parse '{}' (cgltf error {})", path, errorCode));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void GltfImportDiagnosticLog::ReportBufferLoadFailure(std::string_view path, int errorCode, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Failed to load buffers for '{}' (cgltf error {})", path, errorCode));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void GltfImportDiagnosticLog::ReportValidationWarning(std::string_view path, int errorCode, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Validation warnings for '{}' (cgltf error {})", path, errorCode));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportNoSupportedMeshPrimitives(const std::filesystem::path& filePath, SourceImportResult& result)
{
	SPDLOG_LOGGER_ERROR(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: No supported mesh primitives found in '{}'", filePath.string()));
	SourceImportDiagnosticsRecorder::RecordError(result);
}

void GltfImportDiagnosticLog::ReportStaticSkinnedNodes(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: {} skinned nodes are present and will be imported as static data only", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportIgnoredWeightedNodes(std::size_t count, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: {} weighted nodes are present and morph weights will be ignored", count));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportSkippedNonTrianglePrimitive(std::string_view primitiveLabel, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_gltfImportDiagnosticLogger, "{}", std::format("GltfImporter: Skipping {} because only triangle primitives are supported", primitiveLabel));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportIgnoredMorphTargets(std::string_view primitiveLabel, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(g_gltfImportDiagnosticLogger, "{}", std::format("GltfImporter: {} contains morph targets which will be ignored", primitiveLabel));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportSkippedDracoPrimitive(std::string_view primitiveLabel, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Skipping {} because Draco-compressed primitives are not supported yet", primitiveLabel));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportSkippedIncompletePrimitive(std::string_view primitiveLabel, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Skipping {} because vertex or index data is incomplete", primitiveLabel));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportInvalidMaterialIndex(std::string_view primitiveLabel, std::uint32_t materialIndex, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: {} references invalid material index {} and will use the default material", primitiveLabel, materialIndex));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportMalformedGpuInstancing(std::string_view nodeLabel, std::string_view reason, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Node '{}' has malformed EXT_mesh_gpu_instancing data ({}) and will import as a regular mesh node", nodeLabel, reason));
	SourceImportDiagnosticsRecorder::RecordWarning(result);
}

void GltfImportDiagnosticLog::ReportInvalidTexturePath(
    std::uint32_t materialIndex,
    std::string_view slotName,
    std::string_view texturePath,
    SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Material handle {} has an invalid {} texture path '{}' and it will be ignored", materialIndex, slotName, texturePath));
	SourceImportDiagnosticsRecorder::RecordInvalidTexturePath(result);
}

void GltfImportDiagnosticLog::ReportEmbeddedTexture(std::uint32_t materialIndex, std::string_view slotName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Material handle {} uses an embedded {} texture which is not supported yet", materialIndex, slotName));
	SourceImportDiagnosticsRecorder::RecordEmbeddedTextureBinding(result);
}

void GltfImportDiagnosticLog::ReportUnsupportedEncodedTextureSources(std::uint32_t materialIndex, std::string_view slotName, SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format("GltfImporter: Material handle {} uses {} texture sources that are not supported by the runtime importer yet", materialIndex, slotName));
	SourceImportDiagnosticsRecorder::RecordUnsupportedTextureBinding(result);
}

void GltfImportDiagnosticLog::ReportUnsupportedMaterialFeatures(
    std::uint32_t materialIndex,
    std::string_view unsupportedFeatures,
    SourceImportResult& result)
{
	SPDLOG_LOGGER_WARN(
	    g_gltfImportDiagnosticLogger,
	    "{}",
	    std::format(
	        "GltfImporter: Material handle {} uses unsupported glTF material features [{}] and will be approximated with Sparkle PBR defaults",
	        materialIndex,
	        unsupportedFeatures));
	SourceImportDiagnosticsRecorder::RecordUnsupportedMaterialFeature(result);
}
