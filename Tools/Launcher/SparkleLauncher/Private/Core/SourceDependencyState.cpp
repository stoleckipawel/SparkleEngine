#include "SparkleLauncher/SourceDependencyState.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <cstdint>
#include <sstream>
#include <system_error>

namespace SparkleLauncher
{

	bool PathHasContent(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode) || errorCode)
		{
			return false;
		}

		if (std::filesystem::is_regular_file(path, errorCode))
		{
			const std::uintmax_t size = std::filesystem::file_size(path, errorCode);
			return !errorCode && size > 0;
		}

		return true;
	}

	std::string JoinPaths(const std::vector<std::string>& paths)
	{
		std::ostringstream stream;
		for (std::size_t index = 0; index < paths.size(); ++index)
		{
			if (index > 0)
			{
				stream << ", ";
			}
			stream << paths[index];
		}
		return stream.str();
	}

	const std::vector<SourceDependencyEntry>& GetSourceDependencies()
	{
		static const std::vector<SourceDependencyEntry> dependencies = []
		{
			const WorkspaceFeatureSettings features = GetLauncherWorkspaceFeatureSettings();
			return std::vector<SourceDependencyEntry>{
			    {"imgui",
			        "Dear ImGui",
			        "v1.92.5",
			        "Immediate-mode UI core and Win32 platform backend.",
			        "imgui-src",
			        {"imgui.h", "imgui.cpp", "backends/imgui_impl_win32.cpp"},
			        true,
			        true},
			    {"spdlog", "spdlog", "v1.14.1", "Repo-wide logging backend.", "spdlog-src", {"include/spdlog/spdlog.h"}, true, true},
			    {"editor-icons",
			        "Font Awesome Free Solid",
			        "v6.7.1",
			        "Launcher/editor icon font asset and license.",
			        "editor-icons",
			        {"fontawesome-6.7.1/fa-solid-900.ttf", "fontawesome-6.7.1/LICENSE.txt"},
			        true,
			        true},
			    {"cgltf",
			        "cgltf",
			        "v1.15",
			        "Single-header glTF 2.0 parser for source scene imports.",
			        "cgltf-src",
			        {"cgltf.h"},
			        false,
			        features.ContentPipelineEnabled},
			    {"stb",
			        "stb",
			        "master",
			        "Header-only image loading and mip resize helpers.",
			        "stb-src",
			        {"stb_image.h", "stb_image_resize2.h"},
			        false,
			        features.ContentPipelineEnabled},
			    {"tinyexr",
			        "tinyexr",
			        "v1.0.7",
			        "Header-only OpenEXR image loading support.",
			        "tinyexr-src",
			        {"tinyexr.h", "deps/miniz/miniz.h"},
			        false,
			        features.ContentPipelineEnabled},
			    {"zlib",
			        "zlib",
			        "v1.3.1",
			        "Compression backend used by Assimp.",
			        "zlib-src",
			        {"zlib.h", "CMakeLists.txt"},
			        false,
			        features.ContentPipelineEnabled},
			    {"assimp",
			        "Assimp",
			        "v5.4.3",
			        "FBX and DCC scene import support.",
			        "assimp-src",
			        {"include/assimp/Importer.hpp", "CMakeLists.txt"},
			        false,
			        features.ContentPipelineEnabled},
			    {"compressonator",
			        "Compressonator",
			        "master (sparse)",
			        "AMD BC1-BC7 texture block compression support.",
			        "compressonator-src",
			        {"cmp_core/source/cmp_core.cpp", "applications/_libs/cmp_math/cmp_math_common.cpp"},
			        false,
			        features.ContentPipelineEnabled},
			    {"ktx",
			        "KTX-Software",
			        "v4.3.2",
			        "KTX2 texture container read/write support.",
			        "ktx-src",
			        {"include/ktx.h", "CMakeLists.txt"},
			        false,
			        features.KtxSupportEnabled},
			    {"spirv-reflect",
			        "SPIRV-Reflect",
			        "vulkan-sdk-1.3.290.0",
			        "SPIR-V reflection for offline shader compiler backends.",
			        "spirv_reflect-src",
			        {"spirv_reflect.h", "spirv_reflect.c"},
			        false,
			        features.ShaderCompilerEnabled},
			    {"nvidia-nvapi",
			        "NVIDIA NVAPI SDK",
			        "git 9b181ea",
			        "Headers and import library used by D3D12 NVAPI integration and PTLAS capability support.",
			        "sparkle_nvapi-src",
			        {"nvapi.h", "amd64/nvapi64.lib"},
			        false,
			        features.NvidiaStreamlineEnabled},
			    {"nvidia-streamline",
			        "NVIDIA Streamline SDK",
			        "v2.11.1",
			        "Headers, import library, Streamline plugins, and DLSS runtime redistributables.",
			        "streamline-sdk-src",
			        {"include/sl.h",
			            "lib/x64/sl.interposer.lib",
			            "bin/x64/sl.interposer.dll",
			            "bin/x64/sl.common.dll",
			            "bin/x64/sl.dlss.dll",
			            "bin/x64/sl.dlss_d.dll",
			            "bin/x64/nvngx_dlss.dll",
			            "bin/x64/nvngx_dlssd.dll"},
			        false,
			        features.NvidiaStreamlineEnabled},
			};
		}();
		return dependencies;
	}

	const SourceDependencyEntry* FindSourceDependency(std::string_view id)
	{
		for (const SourceDependencyEntry& dependency : GetSourceDependencies())
		{
			if (dependency.Id == id)
			{
				return &dependency;
			}
		}
		return nullptr;
	}

	SourceDependencyValidation ValidateSourceDependency(
	    const SourceDependencyEntry& dependency,
	    const std::filesystem::path& dependencyCacheRoot)
	{
		SourceDependencyValidation validation;
		validation.CachePath = dependencyCacheRoot / dependency.CacheDirectoryName;
		for (const std::string& relativePath : dependency.RequiredRelativePaths)
		{
			if (!PathHasContent(validation.CachePath / relativePath))
			{
				validation.MissingRelativePaths.push_back(relativePath);
			}
		}

		validation.Ready = validation.MissingRelativePaths.empty();
		return validation;
	}

	std::vector<std::filesystem::path> GetSourceDependencyCachePaths(
	    const SourceDependencyEntry& dependency,
	    const std::filesystem::path& dependencyCacheRoot)
	{
		std::vector<std::filesystem::path> paths{dependencyCacheRoot / dependency.CacheDirectoryName};
		constexpr std::string_view sourceSuffix = "-src";
		if (dependency.CacheDirectoryName.ends_with(sourceSuffix))
		{
			const std::string stem = dependency.CacheDirectoryName.substr(0, dependency.CacheDirectoryName.size() - sourceSuffix.size());
			paths.push_back(dependencyCacheRoot / (stem + "-build"));
			paths.push_back(dependencyCacheRoot / (stem + "-subbuild"));
		}
		return paths;
	}

	SourceDependencyInventoryStatus InspectSourceDependencyCache(const std::filesystem::path& dependencyCacheRoot)
	{
		SourceDependencyInventoryStatus status;
		status.CacheRoot = dependencyCacheRoot;

		for (const SourceDependencyEntry& dependency : GetSourceDependencies())
		{
			if (!dependency.Enabled)
			{
				continue;
			}

			++status.EnabledDependencyCount;
			const SourceDependencyValidation validation = ValidateSourceDependency(dependency, dependencyCacheRoot);
			if (validation.Ready)
			{
				++status.ReadyDependencyCount;
				continue;
			}

			status.AllEnabledDependenciesReady = false;
			status.ReadinessMessages.push_back(
			    dependency.Label + " cache is incomplete. Missing: " + JoinPaths(validation.MissingRelativePaths) + ".");
		}

		if (status.EnabledDependencyCount == 0)
		{
			status.AllEnabledDependenciesReady = true;
		}

		return status;
	}
}
