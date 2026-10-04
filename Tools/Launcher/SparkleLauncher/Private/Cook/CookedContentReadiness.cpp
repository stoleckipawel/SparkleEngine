#include "CookedContentReadiness.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/WorkspaceOutputPaths.h"

#include <array>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static bool DirectoryHasRegularFiles(const std::filesystem::path& directory)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(directory, errorCode))
		{
			return false;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    directory,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::directory_entry entry = *iterator;
			if (entry.is_regular_file(errorCode))
			{
				return true;
			}
			errorCode.clear();
			iterator.increment(errorCode);
			errorCode.clear();
		}
		return false;
	}

	static bool CookedAssetScopeHasFiles(
	    const Filesystem::WorkspaceOutputPaths& outputs,
	    std::string_view projectId,
	    std::string_view relativeDirectory)
	{
		const std::string relativeScope(relativeDirectory);
		return DirectoryHasRegularFiles(outputs.CookedProjectDirectory(projectId) / relativeScope)
		    || DirectoryHasRegularFiles(outputs.SharedCookedProjectDirectory() / relativeScope);
	}

	static bool ShaderPublicationMatchesSignal(const std::filesystem::path& cookedShaderDirectory)
	{
		const std::filesystem::path mapPath = Filesystem::BuildGlobalShaderMapPath(cookedShaderDirectory);
		const std::filesystem::path libraryPath = Filesystem::BuildCookedShaderLibraryPath(cookedShaderDirectory);
		std::string signal;
		std::string fileError;
		if (!Files::TryReadAllText(Filesystem::BuildShaderRecookSignalPath(cookedShaderDirectory), signal, fileError))
		{
			return false;
		}

		std::string schema;
		std::string status;
		std::string expectedMapHashText;
		std::string expectedLibraryHashText;
		if (!Json::TryReadStringProperty(signal, "schema", schema) || schema != "sparkle.shaderRecookResult"
		    || !Json::TryReadStringProperty(signal, "status", status) || status != "succeeded"
		    || !Json::TryReadStringProperty(signal, "globalShaderMapHash", expectedMapHashText)
		    || !Json::TryReadStringProperty(signal, "cookedShaderLibraryHash", expectedLibraryHashText))
		{
			return false;
		}

		std::uint64_t expectedMapHash = 0;
		std::uint64_t expectedLibraryHash = 0;
		if (!Json::TryParseHexUInt64(expectedMapHashText, expectedMapHash)
		    || !Json::TryParseHexUInt64(expectedLibraryHashText, expectedLibraryHash))
		{
			return false;
		}

		std::vector<std::uint8_t> mapBytes;
		std::vector<std::uint8_t> libraryBytes;
		return Files::TryReadAllBytes(mapPath, mapBytes, fileError) && Files::TryReadAllBytes(libraryPath, libraryBytes, fileError)
		    && Hash::Fnv1a64(mapBytes.data(), mapBytes.size()) == expectedMapHash
		    && Hash::Fnv1a64(libraryBytes.data(), libraryBytes.size()) == expectedLibraryHash;
	}

	static CookedShaderPublicationState InspectCookedShaderPublication(const std::filesystem::path& cookedShaderDirectory)
	{
		const std::array requiredArtifacts = {
		    Filesystem::BuildGlobalShaderMapPath(cookedShaderDirectory),
		    Filesystem::BuildCookedShaderLibraryPath(cookedShaderDirectory),
		    Filesystem::BuildShaderDependencyManifestPath(cookedShaderDirectory),
		    Filesystem::BuildShaderRecookSignalPath(cookedShaderDirectory)};

		for (const std::filesystem::path& artifactPath : requiredArtifacts)
		{
			std::error_code errorCode;
			if (!std::filesystem::is_regular_file(artifactPath, errorCode) || errorCode
			    || std::filesystem::file_size(artifactPath, errorCode) == 0 || errorCode)
			{
				return CookedShaderPublicationState::Missing;
			}
		}
		if (!ShaderPublicationMatchesSignal(cookedShaderDirectory))
		{
			return CookedShaderPublicationState::Invalid;
		}

		return CookedShaderPublicationState::Ready;
	}

	CookedContentReadiness InspectCookedContentReadiness(const std::filesystem::path& repositoryRoot, std::string_view projectId)
	{
		const Filesystem::WorkspaceOutputPaths outputs = Filesystem::ResolveWorkspaceOutputPaths(repositoryRoot);
		return CookedContentReadiness{
		    .MeshesReady = CookedAssetScopeHasFiles(outputs, projectId, "Meshes"),
		    .TexturesReady = CookedAssetScopeHasFiles(outputs, projectId, "Textures"),
		    .Shaders = InspectCookedShaderPublication(outputs.CookedProjectDirectory(projectId) / "Shaders")};
	}
}
