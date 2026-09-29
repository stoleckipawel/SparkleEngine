#include "CookedContentReadiness.h"

#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonReader.h"
#include "Core/Public/Paths/WorkspaceOutputPaths.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
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

	static std::optional<std::filesystem::file_time_type> FindLatestWriteTime(const std::filesystem::path& root)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode) || errorCode)
		{
			return std::nullopt;
		}

		std::filesystem::file_time_type latestWriteTime = std::filesystem::last_write_time(root, errorCode);
		if (errorCode)
		{
			return std::nullopt;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    root,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		if (errorCode)
		{
			return std::nullopt;
		}

		while (iterator != end)
		{
			const std::filesystem::file_time_type writeTime = iterator->last_write_time(errorCode);
			if (errorCode)
			{
				return std::nullopt;
			}
			latestWriteTime = std::max(latestWriteTime, writeTime);
			iterator.increment(errorCode);
			if (errorCode)
			{
				return std::nullopt;
			}
		}
		return latestWriteTime;
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
		return Files::TryReadAllBytes(mapPath, mapBytes, fileError)
		    && Files::TryReadAllBytes(libraryPath, libraryBytes, fileError)
		    && Hash::Fnv1a64(mapBytes.data(), mapBytes.size()) == expectedMapHash
		    && Hash::Fnv1a64(libraryBytes.data(), libraryBytes.size()) == expectedLibraryHash;
	}

	static CookedOutputState InspectCookedShaders(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& cookedShaderDirectory,
	    std::string_view projectId)
	{
		const std::array requiredArtifacts = {
		    Filesystem::BuildGlobalShaderMapPath(cookedShaderDirectory),
		    Filesystem::BuildCookedShaderLibraryPath(cookedShaderDirectory),
		    Filesystem::BuildShaderDependencyManifestPath(cookedShaderDirectory),
		    Filesystem::BuildShaderRecookSignalPath(cookedShaderDirectory)};

		std::optional<std::filesystem::file_time_type> oldestArtifactWriteTime;
		for (const std::filesystem::path& artifactPath : requiredArtifacts)
		{
			std::error_code errorCode;
			if (!std::filesystem::is_regular_file(artifactPath, errorCode) || errorCode
			    || std::filesystem::file_size(artifactPath, errorCode) == 0 || errorCode)
			{
				return CookedOutputState::Missing;
			}

			const std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(artifactPath, errorCode);
			if (errorCode)
			{
				return CookedOutputState::Stale;
			}
			oldestArtifactWriteTime = oldestArtifactWriteTime.has_value()
			    ? std::min(*oldestArtifactWriteTime, writeTime)
			    : writeTime;
		}
		if (!ShaderPublicationMatchesSignal(cookedShaderDirectory))
		{
			return CookedOutputState::Stale;
		}

		const std::array sourceRoots = {
		    repositoryRoot / "Engine" / "Assets" / "Shaders",
		    repositoryRoot / "Projects" / std::string(projectId) / "Assets" / "Shaders"};
		for (std::size_t sourceRootIndex = 0; sourceRootIndex < sourceRoots.size(); ++sourceRootIndex)
		{
			const std::filesystem::path& sourceRoot = sourceRoots[sourceRootIndex];
			std::error_code errorCode;
			const bool exists = std::filesystem::exists(sourceRoot, errorCode);
			if (errorCode || (!exists && sourceRootIndex == 0))
			{
				return CookedOutputState::Stale;
			}
			if (!exists)
			{
				continue;
			}

			const std::optional<std::filesystem::file_time_type> latestSourceWriteTime = FindLatestWriteTime(sourceRoot);
			if (!latestSourceWriteTime.has_value() || *latestSourceWriteTime > *oldestArtifactWriteTime)
			{
				return CookedOutputState::Stale;
			}
		}

		return CookedOutputState::Ready;
	}

	CookedContentReadiness InspectCookedContentReadiness(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectId)
	{
		const Filesystem::WorkspaceOutputPaths outputs = Filesystem::ResolveWorkspaceOutputPaths(repositoryRoot);
		return CookedContentReadiness{
		    .MeshesReady = CookedAssetScopeHasFiles(outputs, projectId, "Meshes"),
		    .TexturesReady = CookedAssetScopeHasFiles(outputs, projectId, "Textures"),
		    .Shaders = InspectCookedShaders(
		        repositoryRoot,
		        outputs.CookedProjectDirectory(projectId) / "Shaders",
		        projectId)};
	}
}
