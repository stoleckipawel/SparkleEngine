#include "PCH.h"

#include "SourceSceneImporter.h"

#include "SourceImporter.h"
#include "Fbx/FbxImporter.h"
#include "Gltf/GltfImporter.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Diagnostics/SourceImportDiagnosticLog.h"

#include <array>

bool SourceSceneImporter::SupportsSourceScenePath(const std::filesystem::path& filePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	static const GltfImporter gltfImporter;
	static const FbxImporter fbxImporter;
	const std::array<const SourceImporter*, 2> importers = {&gltfImporter, &fbxImporter};

	for (const SourceImporter* importer : importers)
	{
		if (importer->SupportsExtension(extension))
		{
			return true;
		}
	}

	return false;
}

SourceImportResult SourceSceneImporter::Import(const std::filesystem::path& filePath)
{
	SourceImportResult result;
	bool handledByImporter = false;

	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	static const GltfImporter gltfImporter;
	static const FbxImporter fbxImporter;
	const std::array<const SourceImporter*, 2> importers = {&gltfImporter, &fbxImporter};

	for (const SourceImporter* importer : importers)
	{
		if (!importer->SupportsExtension(extension))
		{
			continue;
		}

		result = importer->Import(filePath);
		handledByImporter = true;
		break;
	}

	if (!handledByImporter)
	{
		result.report.sourcePath = filePath;
		result.report.importerId = "UnsupportedSourceImporter";
		SourceImportDiagnosticLog::ReportUnsupportedExtension(extension, filePath, result);
	}

	return result;
}


