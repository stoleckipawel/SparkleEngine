#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerOffscreenRequest.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerOffscreenManifest.h"

#include <fstream>
#include <iterator>
#include <optional>
#include <string_view>

bool ReadReferencePathTracerOffscreenRequest(
    const std::filesystem::path& path,
    ReferencePathTracerOffscreenRequest& request,
    std::string& errorMessage)
{
	request = {};
	std::error_code fileError;
	const std::uintmax_t fileSize = std::filesystem::file_size(path, fileError);
	if (fileError || fileSize > 1024u * 1024u)
	{
		errorMessage = fileError ? "manifest-open-failed" : "manifest-size-invalid";
		return false;
	}
	std::ifstream input(path, std::ios::binary);
	if (!input.is_open())
	{
		errorMessage = "manifest-open-failed";
		return false;
	}
	const std::string document{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
	if (!input.eof()
	    || (document.size() >= 3u && static_cast<unsigned char>(document[0]) == 0xefu && static_cast<unsigned char>(document[1]) == 0xbbu
	        && static_cast<unsigned char>(document[2]) == 0xbfu)
	    || !Strings::IsValidUtf8(document))
	{
		errorMessage = "request-encoding-invalid";
		return false;
	}

	ReferencePathTracerOffscreenManifest manifest;
	std::string schema;
	std::string project;
	std::string level;
	std::string camera;
	std::string product = "SurfaceTransportReference";
	std::string backend;
	std::string outputDirectory;
	std::string filter = "Box";
	std::string checkpointPolicy = "None";
	std::uint64_t width = 0u;
	std::uint64_t height = 0u;
	std::uint64_t targetSpp = 0u;
	std::uint64_t seed = 0u;
	std::uint64_t replicate = 0u;
	std::uint64_t timeoutSeconds = 28800u;
	std::uint64_t maximumOutputGiB = 64u;
	if (!manifest.Parse(document, errorMessage) || !manifest.Read("schema", schema) || schema != "sparkle.reference-path-tracer.request/1"
	    || !manifest.Read("project", project) || !manifest.Read("level", level) || !manifest.Read("camera", camera)
	    || !manifest.Read("backend", backend) || !manifest.Read("outputDirectory", outputDirectory) || !manifest.Read("width", width)
	    || !manifest.Read("height", height) || !manifest.Read("targetSpp", targetSpp) || !manifest.ReadOptional("product", product)
	    || !manifest.ReadOptional("filter", filter) || !manifest.ReadOptional("checkpointPolicy", checkpointPolicy)
	    || !manifest.ReadOptional("seed", seed) || !manifest.ReadOptional("replicate", replicate)
	    || !manifest.ReadOptional("timeoutSeconds", timeoutSeconds) || !manifest.ReadOptional("maxOutputGiB", maximumOutputGiB)
	    || width == 0u || width > 16384u || height == 0u || height > 16384u || targetSpp != 4096u || camera != "ActiveGameCamera"
	    || product != "SurfaceTransportReference" || filter != "Box" || seed != 0u || replicate != 0u || timeoutSeconds == 0u
	    || timeoutSeconds > 28800u || maximumOutputGiB == 0u || maximumOutputGiB > 64u || (backend != "D3D12" && backend != "Vulkan")
	    || (checkpointPolicy != "None" && checkpointPolicy != "OnTimeout"))
	{
		errorMessage = errorMessage.empty() ? "manifest-contract-invalid" : errorMessage;
		return false;
	}

	const std::filesystem::path requestParent = std::filesystem::absolute(path).parent_path();
	const std::optional<std::filesystem::path> projectPath = Paths::ResolveRelativePath(requestParent, std::filesystem::u8path(project));
	if (!projectPath || !std::filesystem::exists(*projectPath, fileError) || fileError)
	{
		errorMessage = "project-not-found";
		return false;
	}
	request.ProjectRoot = std::filesystem::is_directory(*projectPath, fileError) ? *projectPath : projectPath->parent_path();
	if (fileError)
	{
		errorMessage = "project-not-found";
		return false;
	}

	fileError.clear();
	const std::optional<std::filesystem::path> levelPath = Paths::ResolveRelativePath(requestParent, std::filesystem::u8path(level));
	if (!levelPath)
	{
		errorMessage = "level-outside-project";
		return false;
	}
	fileError.clear();
	const std::filesystem::path assetsRoot = std::filesystem::weakly_canonical(request.ProjectRoot / "Assets", fileError);
	if (fileError || !Paths::IsUnderRoot(*levelPath, assetsRoot))
	{
		errorMessage = "level-outside-project";
		return false;
	}
	try
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(request.ProjectRoot);
		for (const ProjectLevelCatalogEntry& entry : catalog.levels)
		{
			fileError.clear();
			if (entry.selected && catalog.IsLevelReady(entry)
			    && std::filesystem::weakly_canonical(entry.sourcePath, fileError) == *levelPath && !fileError)
			{
				request.LevelId = entry.id;
				break;
			}
		}
	}
	catch (...)
	{
	}
	if (request.LevelId.empty())
	{
		errorMessage = "level-not-ready";
		return false;
	}

	const std::optional<std::filesystem::path> resolvedOutput =
	    Paths::ResolveRelativePath(requestParent, std::filesystem::u8path(outputDirectory));
	if (!resolvedOutput)
	{
		errorMessage = "output-directory-invalid";
		return false;
	}
	request.OutputDirectory = *resolvedOutput;
	fileError.clear();
	const std::filesystem::path canonicalOutput = std::filesystem::weakly_canonical(request.OutputDirectory, fileError);
	if (fileError)
	{
		errorMessage = "output-directory-invalid";
		return false;
	}
	fileError.clear();
	const std::filesystem::path executableRoot = std::filesystem::weakly_canonical(Filesystem::GetExecutableDirectory(), fileError);
	if (fileError)
	{
		errorMessage = "output-directory-invalid";
		return false;
	}
	fileError.clear();
	const bool outputExists = std::filesystem::exists(request.OutputDirectory, fileError);
	if (fileError || Paths::IsUnderRoot(canonicalOutput, executableRoot)
	    || (outputExists
	        && (!std::filesystem::is_directory(request.OutputDirectory, fileError)
	            || !std::filesystem::is_empty(request.OutputDirectory, fileError))))
	{
		errorMessage = "output-directory-not-empty";
		return false;
	}

	request.Backend = backend == "D3D12" ? ERhiBackendApi::D3D12 : ERhiBackendApi::Vulkan;
	request.Width = static_cast<std::uint32_t>(width);
	request.Height = static_cast<std::uint32_t>(height);
	request.TimeoutSeconds = static_cast<std::uint32_t>(timeoutSeconds);
	request.MaximumOutputBytes = maximumOutputGiB * 1024ull * 1024ull * 1024ull;
	request.CheckpointOnTimeout = checkpointPolicy == "OnTimeout";
	errorMessage.clear();
	return true;
}
