#include "SparkleLauncher/SourceDependencyState.h"

#include "HostGraphicsCapabilities.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <cstdint>
#include <sstream>
#include <system_error>

namespace SparkleLauncher
{
	namespace
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
	}

	const std::vector<SourceDependencyGroup>& GetSourceDependencyGroups()
	{
		static const std::vector<SourceDependencyGroup> groups = [] {
			const WorkspaceFeatureSettings featureSettings = GetLauncherWorkspaceFeatureSettings();
			const HostGraphicsCapabilities& hostGraphics = GetHostGraphicsCapabilities();
			std::vector<SourceDependencyGroup> entries;
			entries.push_back({
			    "core-workspace",
			    "Core build dependencies",
			    "Baseline packages shared by the launcher, engine, and project builds.",
			    "Required for local rebuilds. Packaged binaries can still launch without rebuilding from source.",
			    "",
			    "Always enabled for local source workflows.",
			    true,
			    true,
			    {
			        {"imgui", "Dear ImGui", "v1.92.5", "Immediate-mode UI core and Win32 platform backend.", "imgui-src", {"imgui.h", "imgui.cpp", "backends/imgui_impl_win32.cpp"}},
			        {"spdlog", "spdlog", "v1.14.1", "Repo-wide logging backend.", "spdlog-src", {"include/spdlog/spdlog.h"}},
			        {"editor-icons", "Font Awesome Free Solid", "v6.7.1", "Launcher/editor icon font asset and license.", "editor-icons", {"fontawesome-6.7.1/fa-solid-900.ttf", "fontawesome-6.7.1/LICENSE.txt"}},
			    }});
			const bool contentPipelineEnabled = featureSettings.ContentPipelineEnabled;
			entries.push_back({
			    "content-pipeline",
			    "Content pipeline tools",
			    "Optional packages for source import, mesh cooking, and texture cooking.",
			    "Enables Build Cooking Tools, Cook Textures, Cook Scenes And Meshes, and the content phase of Cook All.",
			    "SPARKLE_ENABLE_CONTENT_PIPELINE",
			    contentPipelineEnabled ? "Enabled by workspace feature settings." : "Skipped because the content-pipeline workspace feature is off.",
			    false,
			    contentPipelineEnabled,
			    {
			        {"cgltf", "cgltf", "v1.15", "Single-header glTF 2.0 parser for source scene imports.", "cgltf-src", {"cgltf.h"}},
			        {"stb", "stb", "master", "Header-only image loading and mip resize helpers.", "stb-src", {"stb_image.h", "stb_image_resize2.h"}},
			        {"tinyexr", "tinyexr", "v1.0.7", "Header-only OpenEXR image loading support.", "tinyexr-src", {"tinyexr.h", "deps/miniz/miniz.h"}},
			        {"zlib", "zlib", "v1.3.1", "Compression backend used by Assimp.", "zlib-src", {"zlib.h", "CMakeLists.txt"}},
			        {"assimp", "Assimp", "v5.4.3", "FBX and DCC scene import support.", "assimp-src", {"include/assimp/Importer.hpp", "CMakeLists.txt"}},
			        {"compressonator", "Compressonator", "master (sparse)", "AMD BC1-BC7 texture block compression support.", "compressonator-src", {"cmp_core/source/cmp_core.cpp", "applications/_libs/cmp_math/cmp_math_common.cpp"}},
			    }});
			const bool ktxSupportEnabled = featureSettings.KtxSupportEnabled;
			entries.push_back({
			    "ktx-support",
			    "KTX texture support",
			    "Optional KTX2 container support layered on top of the texture pipeline.",
			    "Extends texture workflows when KTX support is enabled for this workspace.",
			    "SPARKLE_ENABLE_KTX_SUPPORT",
			    ktxSupportEnabled ? "Enabled by workspace feature settings." : "Skipped because KTX support is off.",
			    false,
			    ktxSupportEnabled,
			    {
			        {"ktx", "KTX-Software", "v4.3.2", "KTX2 texture container read/write support.", "ktx-src", {"include/ktx.h", "CMakeLists.txt"}},
			    }});
			const bool shaderCompilerEnabled = featureSettings.ShaderCompilerEnabled;
			entries.push_back({
			    "shader-compiler",
			    "Shader compiler support",
			    "Optional offline shader compiler dependencies.",
			    "Enables Build Cooking Tools, Cook Shaders, and the shader phase of Cook All.",
			    "SPARKLE_ENABLE_SHADER_COMPILER",
			    shaderCompilerEnabled ? "Enabled by workspace feature settings." : "Skipped because the shader-compiler workspace feature is off.",
			    false,
			    shaderCompilerEnabled,
			    {
			        {"spirv-reflect", "SPIRV-Reflect", "vulkan-sdk-1.3.290.0", "SPIR-V reflection for offline shader compiler backends.", "spirv_reflect-src", {"spirv_reflect.h", "spirv_reflect.c"}},
			    }});
			const bool nvidiaStreamlineEnabled = featureSettings.NvidiaStreamlineEnabled;
			entries.push_back({
			    "nvidia-streamline",
			    "NVIDIA DLSS and NVAPI",
			    "Optional NVIDIA SDK dependencies used by DLSS and D3D12 NVAPI integration.",
			    "Stages Streamline and DLSS runtime files, keeps NVAPI/PTLAS source integration available for local rebuilds, and expects the Vulkan SDK on the host for renderer builds.",
			    "SPARKLE_ENABLE_NVIDIA_STREAMLINE",
			    nvidiaStreamlineEnabled ? hostGraphics.Summary :
			                             (hostGraphics.HasAmdAdapter || hostGraphics.HasIntelAdapter || hostGraphics.AdapterDetected ?
			                                  "Skipped because no NVIDIA graphics adapter was detected on this machine." :
			                                  hostGraphics.Summary),
			    false,
			    nvidiaStreamlineEnabled,
			    {
			        {"nvidia-nvapi", "NVIDIA NVAPI SDK", "git 9b181ea", "Headers and import library used by D3D12 NVAPI integration and PTLAS capability support.", "sparkle_nvapi-src", {"nvapi.h", "amd64/nvapi64.lib"}},
			        {"nvidia-streamline", "NVIDIA Streamline SDK", "v2.11.1", "Headers, import library, Streamline plugins, and DLSS runtime redistributables.", "streamline-sdk-src", {"include/sl.h", "lib/x64/sl.interposer.lib", "bin/x64/sl.interposer.dll", "bin/x64/sl.common.dll", "bin/x64/sl.dlss.dll", "bin/x64/sl.dlss_d.dll", "bin/x64/nvngx_dlss.dll", "bin/x64/nvngx_dlssd.dll"}},
			    }});
			return entries;
		}();
		return groups;
	}

	const SourceDependencyGroup* FindSourceDependencyGroup(std::string_view id)
	{
		for (const SourceDependencyGroup& group : GetSourceDependencyGroups())
		{
			if (group.Id == id)
			{
				return &group;
			}
		}
		return nullptr;
	}

	const SourceDependencyEntry* FindSourceDependency(std::string_view id)
	{
		for (const SourceDependencyGroup& group : GetSourceDependencyGroups())
		{
			for (const SourceDependencyEntry& dependency : group.Dependencies)
			{
				if (dependency.Id == id)
				{
					return &dependency;
				}
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

	int CountReadySourceDependencies(const SourceDependencyGroup& group, const std::filesystem::path& dependencyCacheRoot)
	{
		int readyCount = 0;
		for (const SourceDependencyEntry& dependency : group.Dependencies)
		{
			if (ValidateSourceDependency(dependency, dependencyCacheRoot).Ready)
			{
				++readyCount;
			}
		}
		return readyCount;
	}

	SourceDependencyInventoryStatus InspectSourceDependencyCache(const std::filesystem::path& dependencyCacheRoot)
	{
		SourceDependencyInventoryStatus status;
		status.CacheRoot = dependencyCacheRoot;

		for (const SourceDependencyGroup& group : GetSourceDependencyGroups())
		{
			if (!group.Enabled)
			{
				continue;
			}

			for (const SourceDependencyEntry& dependency : group.Dependencies)
			{
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
		}

		if (status.EnabledDependencyCount == 0)
		{
			status.AllEnabledDependenciesReady = true;
		}

		return status;
	}
}
