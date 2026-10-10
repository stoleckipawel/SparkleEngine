#include "PCH.h"

#include "SourceSceneImporter.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Fbx/FbxImporter.h"
#include "Gltf/GltfImporter.h"
#include "Ply/PlyImporter.h"

#include <algorithm>
#include <array>
#include <format>
#include <string_view>

struct SourceSceneFormat final
{
	std::wstring_view Extension;
	std::string_view ImporterId;
	SourceImportOutput (*Import)(const std::filesystem::path&);
};

static constexpr std::array SourceSceneFormats = {
    SourceSceneFormat{L".gltf", "GltfImporter", ImportGltfScene},
    SourceSceneFormat{L".glb", "GltfImporter", ImportGltfScene},
    SourceSceneFormat{L".fbx", "FbxImporter", ImportFbxScene},
    SourceSceneFormat{L".ply", "PlyImporter", ImportPlyScene}};

bool SupportsSourceScenePath(const std::filesystem::path& filePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	return std::ranges::any_of(SourceSceneFormats, [&extension](const SourceSceneFormat& format) { return format.Extension == extension; });
}

SourceImportOutput ImportSourceScene(const std::filesystem::path& filePath)
{
	const std::wstring extension = Paths::GetLowercaseExtension(filePath);
	for (const SourceSceneFormat& format : SourceSceneFormats)
	{
		if (format.Extension == extension)
		{
			SourceImportOutput output = format.Import(filePath);
			output.provenance.importerId = format.ImporterId;
			return output;
		}
	}
	throw Diagnostics::Error(std::format("No source scene importer supports extension '{}' for '{}'.", extension.empty() ? std::string("<none>") : Strings::ToNarrow(extension), filePath.string()));
}
