#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "BuildFreshnessSignature.h"
#include "CMakeGeneratorModel.h"
#include "BuildWorkspaceStateFiles.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <fstream>
#include <optional>
#include <system_error>

namespace SparkleLauncher
{
	static std::string MakeFreshnessRelativePath(const std::filesystem::path& repositoryRoot, const std::filesystem::path& path)
	{
		const std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(path, repositoryRoot);
		std::string value = relativePath.has_value() ? relativePath->generic_string() : path.filename().generic_string();
		std::replace(value.begin(), value.end(), '/', '\\');
		return Strings::ToLowerCopy(value);
	}

	static std::string ToCMakeBool(bool value)
	{
		return value ? "ON" : "OFF";
	}

	BuildFilesFreshnessStatus CheckBuildFilesFreshness(const std::filesystem::path& repositoryRoot, const BuildToolchainStatus& toolchain)
	{
		BuildFilesFreshnessStatus status;
		status.BuildDirectory = GetBuildDirectory(repositoryRoot);
		status.CachePath = status.BuildDirectory / "CMakeCache.txt";
		status.SolutionPath = GetBuildSolutionPath(repositoryRoot);
		status.StampPath = status.BuildDirectory / "BuildFilesFreshness.json";

		std::error_code errorCode;
		if (!std::filesystem::is_directory(status.BuildDirectory, errorCode))
		{
			status.State = BuildFilesFreshnessState::BuildDirectoryMissing;
			status.Summary = "Build directory is missing.";
			return status;
		}

		if (!std::filesystem::is_regular_file(status.CachePath, errorCode))
		{
			status.State = BuildFilesFreshnessState::CMakeCacheMissing;
			status.Summary = "CMake cache is missing.";
			return status;
		}

		if (CMakeGeneratorProducesSolution(toolchain.Generator) && !std::filesystem::is_regular_file(status.SolutionPath, errorCode))
		{
			status.State = BuildFilesFreshnessState::SolutionMissing;
			status.Summary = "Solution file is missing.";
			return status;
		}

		const std::string cacheGenerator = ReadCMakeCacheValue(status.CachePath, "CMAKE_GENERATOR").value_or(std::string());
		const std::string cachePlatform = ReadCMakeCacheValue(status.CachePath, "CMAKE_GENERATOR_PLATFORM").value_or(std::string());
		const std::string cacheToolset = ReadCMakeCacheValue(status.CachePath, "CMAKE_GENERATOR_TOOLSET").value_or(std::string());
		const std::string cacheQtPrefixPath = ReadCMakeCacheValue(status.CachePath, "CMAKE_PREFIX_PATH").value_or(std::string());
		const std::string selectedPlatform = GetCMakeCachePlatformValue(toolchain);
		const std::string selectedToolset = GetCMakeCacheToolsetValue(toolchain);
		const std::string selectedQtPrefixPath = toolchain.QtRootPath.generic_string();
		const std::string selectedVulkanSdkRoot = toolchain.VulkanSdkRoot.generic_string();
		if (cacheGenerator != toolchain.Generator || cachePlatform != selectedPlatform || cacheToolset != selectedToolset
		    || cacheQtPrefixPath != selectedQtPrefixPath)
		{
			status.State = BuildFilesFreshnessState::GeneratorMismatch;
			status.Summary = "CMake cache generator/platform/toolset/Qt prefix differs from selected launcher toolchain.";
			return status;
		}

		const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();
		const std::string selectedContentPipeline = ToCMakeBool(featureSettings.ContentPipelineEnabled);
		const std::string selectedShaderCompiler = ToCMakeBool(featureSettings.ShaderCompilerEnabled);
		const std::string selectedKtxSupport = ToCMakeBool(featureSettings.KtxSupportEnabled);
		const std::string selectedNvidiaStreamline = ToCMakeBool(featureSettings.NvidiaStreamlineEnabled);
		const std::string cacheContentPipeline =
		    ReadCMakeCacheValue(status.CachePath, "SPARKLE_ENABLE_CONTENT_PIPELINE").value_or(std::string());
		const std::string cacheShaderCompiler =
		    ReadCMakeCacheValue(status.CachePath, "SPARKLE_ENABLE_SHADER_COMPILER").value_or(std::string());
		const std::string cacheKtxSupport = ReadCMakeCacheValue(status.CachePath, "SPARKLE_ENABLE_KTX_SUPPORT").value_or(std::string());
		const std::string cacheNvidiaStreamline =
		    ReadCMakeCacheValue(status.CachePath, "SPARKLE_ENABLE_NVIDIA_STREAMLINE").value_or(std::string());
		if (cacheContentPipeline != selectedContentPipeline || cacheShaderCompiler != selectedShaderCompiler
		    || cacheKtxSupport != selectedKtxSupport || cacheNvidiaStreamline != selectedNvidiaStreamline)
		{
			status.State = BuildFilesFreshnessState::FeatureSetMismatch;
			status.Summary = "CMake cache workspace feature toggles differ from the launcher feature set.";
			return status;
		}

		if (!std::filesystem::is_regular_file(status.StampPath, errorCode))
		{
			status.State = BuildFilesFreshnessState::FreshnessStampMissing;
			status.Summary = "Build-file freshness stamp is missing.";
			return status;
		}
		if (ReadBuildFilesFreshnessStampValue(status.StampPath, "generator").value_or(std::string()) != toolchain.Generator
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "platform").value_or(std::string()) != toolchain.Platform
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "toolset").value_or(std::string()) != toolchain.Toolset
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "qtRoot").value_or(std::string()) != selectedQtPrefixPath
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "vulkanSdkRoot").value_or(std::string()) != selectedVulkanSdkRoot)
		{
			status.State = BuildFilesFreshnessState::FreshnessStampMismatch;
			status.Summary = "Freshness stamp generator/platform/toolset/Qt prefix/Vulkan SDK differs from selected launcher toolchain.";
			return status;
		}
		if (ReadBuildFilesFreshnessStampValue(status.StampPath, "contentPipeline").value_or(std::string()) != selectedContentPipeline
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "shaderCompiler").value_or(std::string()) != selectedShaderCompiler
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "ktxSupport").value_or(std::string()) != selectedKtxSupport
		    || ReadBuildFilesFreshnessStampValue(status.StampPath, "nvidiaStreamline").value_or(std::string()) != selectedNvidiaStreamline)
		{
			status.State = BuildFilesFreshnessState::FeatureSetMismatch;
			status.Summary = "Build-file freshness stamp workspace feature toggles differ from the launcher feature set.";
			return status;
		}

		const std::optional<std::string> sourceListHash = ComputeSourceListHash(repositoryRoot);
		if (!sourceListHash.has_value())
		{
			status.State = BuildFilesFreshnessState::Unsupported;
			status.Summary = "Native source-list hash is unavailable on this platform.";
			return status;
		}

		if (ReadBuildFilesFreshnessStampValue(status.StampPath, "sourceListHash").value_or(std::string()) != *sourceListHash)
		{
			status.State = BuildFilesFreshnessState::SourceListChanged;
			status.Summary = "Source file list changed since build files were generated.";
			return status;
		}

		const std::filesystem::file_time_type stampTime = std::filesystem::last_write_time(status.StampPath, errorCode);
		for (const std::filesystem::path& path : CollectBuildInputPaths(repositoryRoot))
		{
			const std::filesystem::file_time_type inputTime = std::filesystem::last_write_time(path, errorCode);
			if (!errorCode && inputTime > stampTime)
			{
				status.State = BuildFilesFreshnessState::BuildInputChanged;
				status.Summary = "Build input changed: " + MakeFreshnessRelativePath(repositoryRoot, path);
				return status;
			}
			errorCode.clear();
		}

		status.State = BuildFilesFreshnessState::Current;
		status.Current = true;
		status.Summary = "Build files are current.";
		return status;
	}

	bool UpdateBuildFilesFreshnessStamp(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string& errorMessage)
	{
		const std::optional<std::string> sourceListHash = ComputeSourceListHash(repositoryRoot);
		if (!sourceListHash.has_value())
		{
			errorMessage = "Native source-list hash is unavailable on this platform.";
			return false;
		}

		const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();

		const std::filesystem::path stampPath = GetBuildDirectory(repositoryRoot) / "BuildFilesFreshness.json";
		std::error_code errorCode;
		std::filesystem::create_directories(stampPath.parent_path(), errorCode);
		if (errorCode)
		{
			errorMessage = "Failed to create build directory for freshness stamp.";
			return false;
		}

		std::ofstream stream(stampPath, std::ios::out | std::ios::trunc);
		if (!stream.is_open())
		{
			errorMessage = "Failed to open build freshness stamp for writing.";
			return false;
		}

		stream << "{\n"
		       << "    \"generator\": \"" << Strings::EscapeJsonString(toolchain.Generator) << "\",\n"
		       << "    \"platform\": \"" << Strings::EscapeJsonString(toolchain.Platform) << "\",\n"
		       << "    \"toolset\": \"" << Strings::EscapeJsonString(toolchain.Toolset) << "\",\n"
		       << "    \"qtRoot\": \"" << Strings::EscapeJsonString(toolchain.QtRootPath.generic_string()) << "\",\n"
		       << "    \"vulkanSdkRoot\": \"" << Strings::EscapeJsonString(toolchain.VulkanSdkRoot.generic_string()) << "\",\n"
		       << "    \"contentPipeline\": \"" << ToCMakeBool(featureSettings.ContentPipelineEnabled) << "\",\n"
		       << "    \"shaderCompiler\": \"" << ToCMakeBool(featureSettings.ShaderCompilerEnabled) << "\",\n"
		       << "    \"ktxSupport\": \"" << ToCMakeBool(featureSettings.KtxSupportEnabled) << "\",\n"
		       << "    \"nvidiaStreamline\": \"" << ToCMakeBool(featureSettings.NvidiaStreamlineEnabled) << "\",\n"
		       << "    \"sourceListHash\": \"" << *sourceListHash << "\",\n"
		       << "    \"updatedUtc\": \"" << BuildUtcTimestamp() << "\"\n"
		       << "}\n";

		return true;
	}
}
