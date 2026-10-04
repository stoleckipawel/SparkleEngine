#include "PCH.h"

#include "SourceSceneImporter.h"

#include "SourceImportOutput.h"
#include "SourceImporter.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Fbx/FbxImporter.h"
#include "Gltf/GltfImporter.h"
#include "Ply/PlyImporter.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <string>

bool SourceSceneImporter::SupportsSourceScenePath(const std::filesystem::path& filePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	static const GltfImporter gltfImporter;
	static const FbxImporter fbxImporter;
	static const PlyImporter plyImporter;
	const std::array<const SourceImporter*, 3> importers = {&gltfImporter, &fbxImporter, &plyImporter};

	return std::ranges::any_of(importers, [&extension](const SourceImporter* importer) { return importer->SupportsExtension(extension); });
}

SourceImportOutput SourceSceneImporter::Import(const std::filesystem::path& filePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	static const GltfImporter gltfImporter;
	static const FbxImporter fbxImporter;
	static const PlyImporter plyImporter;
	const std::array<const SourceImporter*, 3> importers = {&gltfImporter, &fbxImporter, &plyImporter};

	for (const SourceImporter* importer : importers)
	{
		if (!importer->SupportsExtension(extension))
		{
			continue;
		}

		return importer->Import(filePath);
	}

	throw Diagnostics::Error(
	    std::format(
	        "No source scene importer supports extension '{}' for '{}'.",
	        extension.empty() ? std::string("<none>") : Strings::ToNarrow(extension),
	        filePath.string()));
}
