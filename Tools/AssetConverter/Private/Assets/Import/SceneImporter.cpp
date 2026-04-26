#include "PCH.h"

#include "Assets/Import/SceneImporter.h"

#include "Assets/Importers/AssetImporter.h"
#include "Assets/Importers/Fbx/FbxImporter.h"
#include "Assets/Importers/Gltf/GltfImporter.h"
#include "Core/Public/Paths/PathUtils.h"

#include <array>
#include <format>

static const auto g_sceneImporterLogger = Logging::GetOrCreateLogger("Tools.AssetConverter");

SceneImportResult SceneImporter::Import(const std::filesystem::path& filePath)
{
	SceneImportResult result;
	bool handledByImporter = false;

	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	static const GltfImporter gltfImporter;
	static const FbxImporter fbxImporter;
	const std::array<const AssetImporter*, 2> importers = {&gltfImporter, &fbxImporter};

	for (const AssetImporter* importer : importers)
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
		SPDLOG_LOGGER_ERROR(
		    g_sceneImporterLogger,
		    "{}",
		    std::format(
		        "SceneImporter: Unsupported asset extension '{}' for '{}'",
		        extension.empty() ? std::string("<none>") : std::string(extension.begin(), extension.end()),
		        filePath.string()));
	}

	return result;
}