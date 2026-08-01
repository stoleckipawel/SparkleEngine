#include "VulkanSdkDiscovery.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static std::optional<std::filesystem::path> TryGetEnvironmentPath(std::string_view variableName)
	{
		std::string value;
		const std::string variableNameText(variableName);
		if (!Environment::TryGetVariable(variableNameText.c_str(), value) || value.empty())
		{
			return std::nullopt;
		}
		return std::filesystem::path(value);
	}

	static std::optional<std::filesystem::path> FindLatestVersionedDirectory(const std::filesystem::path& root)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return std::nullopt;
		}

		std::optional<std::filesystem::path> latestDirectory;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, errorCode))
		{
			if (!entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}
			if (!latestDirectory.has_value() || latestDirectory->filename().string() < entry.path().filename().string())
			{
				latestDirectory = entry.path();
			}
			errorCode.clear();
		}
		return latestDirectory;
	}

	std::optional<std::filesystem::path> DetectInstalledVulkanSdkRoot()
	{
		if (const std::optional<std::filesystem::path> envRoot = TryGetEnvironmentPath("VULKAN_SDK"))
		{
			return envRoot->lexically_normal();
		}
		if (const std::optional<std::filesystem::path> envRoot = TryGetEnvironmentPath("VK_SDK_PATH"))
		{
			return envRoot->lexically_normal();
		}

		const std::array<std::filesystem::path, 3> candidateRoots = {
		    std::filesystem::path("C:\\VulkanSDK"),
		    std::filesystem::path("C:\\Program Files\\VulkanSDK"),
		    std::filesystem::path("C:\\Program Files (x86)\\VulkanSDK"),
		};
		for (const std::filesystem::path& candidateRoot : candidateRoots)
		{
			if (const std::optional<std::filesystem::path> versionRoot = FindLatestVersionedDirectory(candidateRoot))
			{
				return versionRoot->lexically_normal();
			}
		}
		return std::nullopt;
	}

	VulkanSdkStatus DetectVulkanSdk()
	{
		VulkanSdkStatus status;
		const std::optional<std::filesystem::path> sdkRoot = DetectInstalledVulkanSdkRoot();
		if (!sdkRoot.has_value())
		{
			status.Detail = "Vulkan SDK was not detected. Install it or define VULKAN_SDK so Vulkan and NVIDIA Streamline builds can "
			                "resolve Vulkan headers and import libraries.";
			return status;
		}

		status.Root = sdkRoot->lexically_normal();
		const std::array<std::pair<std::filesystem::path, std::string_view>, 2> requiredFiles = {{
		    {status.Root / "Include" / "vulkan" / "vulkan.h", "Include/vulkan/vulkan.h"},
		    {status.Root / "Lib" / "vulkan-1.lib", "Lib/vulkan-1.lib"},
		}};
		std::vector<std::string> missingEntries;
		for (const auto& [path, displayPath] : requiredFiles)
		{
			std::error_code errorCode;
			if (!std::filesystem::exists(path, errorCode))
			{
				missingEntries.emplace_back(displayPath);
			}
		}

		if (missingEntries.empty())
		{
			status.Available = true;
			status.Detail = "Using VULKAN_SDK root: " + status.Root.string();
			return status;
		}

		std::vector<std::string_view> missingEntryViews;
		missingEntryViews.reserve(missingEntries.size());
		for (const std::string& entry : missingEntries)
		{
			missingEntryViews.push_back(entry);
		}
		std::ostringstream detail;
		detail << "Detected Vulkan SDK root " << status.Root.string()
		       << ", but required Vulkan SDK files are missing: " << Strings::Join(missingEntryViews, ", ") << ".";
		status.Detail = detail.str();
		return status;
	}
}
