#include "ShaderCompilerSdkDiscovery.h"

#include "VulkanSdkDiscovery.h"
#include "Core/Public/Strings/StringUtils.h"

#include <array>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static bool HasDirectChildDirectoryWithPrefix(const std::filesystem::path& root, std::string_view prefix)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return false;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, errorCode))
		{
			if (errorCode || !entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}
			const std::string name = Strings::ToLowerCopy(entry.path().filename().string());
			if (name.rfind(Strings::ToLowerCopy(prefix), 0) == 0)
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
			status.Detail = "Vulkan SDK was not detected. Install it or define VULKAN_SDK so the enabled ShaderCompiler workspace tier can "
			                "find DXC and Slang.";
			return status;
		}

		status.Root = sdkRoot->lexically_normal();
		const std::array<std::pair<std::filesystem::path, std::string_view>, 11> requiredFiles = {{
		    {status.Root / "Include" / "dxc" / "dxcapi.h", "Include/dxc/dxcapi.h"},
		    {status.Root / "Lib" / "dxcompiler.lib", "Lib/dxcompiler.lib"},
		    {status.Root / "Bin" / "dxcompiler.dll", "Bin/dxcompiler.dll"},
		    {status.Root / "Include" / "slang" / "slang.h", "Include/slang/slang.h"},
		    {status.Root / "Lib" / "slang.lib", "Lib/slang.lib"},
		    {status.Root / "Bin" / "slang.dll", "Bin/slang.dll"},
		    {status.Root / "Bin" / "slang-compiler.dll", "Bin/slang-compiler.dll"},
		    {status.Root / "Bin" / "slang-glsl-module.dll", "Bin/slang-glsl-module.dll"},
		    {status.Root / "Bin" / "slang-glslang.dll", "Bin/slang-glslang.dll"},
		    {status.Root / "Bin" / "slang-rt.dll", "Bin/slang-rt.dll"},
		    {status.Root / "Bin" / "slang.slang", "Bin/slang.slang"},
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
		if (!HasDirectChildDirectoryWithPrefix(status.Root / "Bin", "slang-standard-module-"))
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
