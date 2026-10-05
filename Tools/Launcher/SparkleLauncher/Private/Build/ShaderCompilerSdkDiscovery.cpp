#include "ShaderCompilerSdkDiscovery.h"

#include "VulkanSdkDiscovery.h"
#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <filesystem>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static constexpr auto shaderCompilerRequiredSdkFiles = std::to_array<std::string_view>(
	    {"Include/dxc/dxcapi.h",
	        "Lib/dxcompiler.lib",
	        "Bin/dxcompiler.dll",
	        "Include/spirv-tools/libspirv.hpp",
	        "Lib/SPIRV-Tools-shared.lib",
	        "Bin/SPIRV-Tools-shared.dll",
	        "Include/slang/slang.h",
	        "Lib/slang.lib",
	        "Bin/slang.dll",
	        "Bin/slang-compiler.dll",
	        "Bin/slang-glsl-module.dll",
	        "Bin/slang-glslang.dll",
	        "Bin/slang-rt.dll",
	        "Bin/slang.slang"});

	std::span<const std::string_view> GetShaderCompilerRequiredSdkFiles() noexcept
	{
		return shaderCompilerRequiredSdkFiles;
	}

	bool HasShaderCompilerStandardModules(const std::filesystem::path& binaryDirectory)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(binaryDirectory, errorCode))
		{
			return false;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(binaryDirectory, errorCode))
		{
			if (errorCode || !entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}
			const std::string name = Strings::ToLowerCopy(entry.path().filename().string());
			if (name.starts_with("slang-standard-module-"))
			{
				return true;
			}
			errorCode.clear();
		}
		return false;
	}

	ShaderCompilerSdkStatus DetectShaderCompilerSdk()
	{
		ShaderCompilerSdkStatus status;
		const std::optional<std::filesystem::path> sdkRoot = DetectInstalledVulkanSdkRoot();
		if (!sdkRoot.has_value())
		{
			status.Detail = "Vulkan SDK was not detected. Install it or define VULKAN_SDK so the enabled ShaderCompiler feature can "
			                "find DXC, Slang and SPIRV-Tools.";
			return status;
		}

		status.Root = sdkRoot->lexically_normal();

		std::vector<std::string> missingEntries;
		for (std::string_view relativePath : GetShaderCompilerRequiredSdkFiles())
		{
			std::error_code errorCode;
			if (!std::filesystem::is_regular_file(status.Root / relativePath, errorCode))
			{
				missingEntries.emplace_back(relativePath);
			}
		}
		if (!HasShaderCompilerStandardModules(status.Root / "Bin"))
		{
			missingEntries.push_back("Bin/slang-standard-module-*");
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
		       << ", but the shader compiler runtime bundle is missing: " << Strings::Join(missingEntryViews, ", ") << ".";
		status.Detail = detail.str();
		return status;
	}
}
