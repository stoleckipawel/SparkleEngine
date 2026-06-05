#include "LauncherDependencyUiModel.h"

#include <system_error>

namespace SparkleLauncher
{
	namespace
	{
		bool DirectoryHasEntries(const std::filesystem::path& path)
		{
			std::error_code errorCode;
			if (!std::filesystem::is_directory(path, errorCode))
			{
				return false;
			}
			return std::filesystem::directory_iterator(path, errorCode) != std::filesystem::directory_iterator();
		}
	}

	const std::vector<DependencyGroupUiEntry>& GetDependencyGroups()
	{
		static const std::vector<DependencyGroupUiEntry> groups = [] {
			std::vector<DependencyGroupUiEntry> entries;
			entries.push_back({
			    "core-workspace",
			    "Core Workspace Source Tier",
			    "Baseline shared dependencies used by the launcher, engine, and project builds.",
			    "Required source tier for local rebuilds. Runtime packages can still launch bundled components without rebuilding this tier.",
			    QString(),
			    true,
			    true,
			    {
			        {"Dear ImGui", "v1.92.5", "Immediate-mode UI core and Win32 platform backend.", "imgui-src"},
			        {"spdlog", "v1.14.1", "Repo-wide logging backend.", "spdlog-src"},
			        {"Font Awesome Free Solid", "v6.7.1", "Launcher/editor icon font asset and license.", "editor-icons"},
			    }});
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			const bool contentPipelineEnabled = true;
#else
			const bool contentPipelineEnabled = false;
#endif
			entries.push_back({
			    "content-pipeline",
			    "Content Pipeline Source Tier",
			    "Optional source import, mesh cook, and texture cook dependencies.",
			    "Unlocks Build Cooking Tools, Cook Textures, Cook Scenes And Meshes, and the content phase of Cook All.",
			    "SPARKLE_ENABLE_CONTENT_PIPELINE",
			    false,
			    contentPipelineEnabled,
			    {
			        {"cgltf", "v1.15", "Single-header glTF 2.0 parser for source scene imports.", "cgltf-src"},
			        {"stb", "master", "Header-only image loading and mip resize helpers.", "stb-src"},
			        {"tinyexr", "v1.0.7", "Header-only OpenEXR image loading support.", "tinyexr-src"},
			        {"zlib", "v1.3.1", "Compression backend used by Assimp.", "zlib-src"},
			        {"Assimp", "v5.4.3", "FBX and DCC scene import support.", "assimp-src"},
			        {"Compressonator", "master (sparse)", "AMD BC1-BC7 texture block compression support.", "compressonator-src"},
			    }});
#if SPARKLE_ENABLE_KTX_SUPPORT
			const bool ktxSupportEnabled = true;
#else
			const bool ktxSupportEnabled = false;
#endif
			entries.push_back({
			    "ktx-support",
			    "KTX Container Source Tier",
			    "Optional KTX2 container support layered on top of the texture pipeline.",
			    "Extends texture workflows when the repo is configured for KTX support.",
			    "SPARKLE_ENABLE_KTX_SUPPORT",
			    false,
			    ktxSupportEnabled,
			    {
			        {"KTX-Software", "v4.3.2", "KTX2 texture container read/write support.", "ktx-src"},
			    }});
#if SPARKLE_ENABLE_SHADER_COMPILER
			const bool shaderCompilerEnabled = true;
#else
			const bool shaderCompilerEnabled = false;
#endif
			entries.push_back({
			    "shader-compiler",
			    "Shader Compiler Source Tier",
			    "Optional offline shader compiler dependencies.",
			    "Unlocks Build Cooking Tools, Cook Shaders, and the shader phase of Cook All.",
			    "SPARKLE_ENABLE_SHADER_COMPILER",
			    false,
			    shaderCompilerEnabled,
			    {
			        {"SPIRV-Reflect", "vulkan-sdk-1.3.290.0", "SPIR-V reflection for offline shader compiler backends.", "spirv_reflect-src"},
			    }});
			return entries;
		}();
		return groups;
	}

	const std::vector<ThirdPartyDependencyUiEntry>& GetTrackedThirdPartyDependencies()
	{
		static const std::vector<ThirdPartyDependencyUiEntry> dependencies = [] {
			std::vector<ThirdPartyDependencyUiEntry> entries;
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				entries.insert(entries.end(), group.Dependencies.begin(), group.Dependencies.end());
			}
			return entries;
		}();
		return dependencies;
	}

	QString FormatTrackedDependencySummary(const std::filesystem::path& dependencyCachePath)
	{
		int readyCount = 0;
		int trackedCount = 0;
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			if (!group.Enabled)
			{
				continue;
			}
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				++trackedCount;
				if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
				{
					++readyCount;
				}
			}
		}

		return QStringLiteral("%1 of %2 enabled tracked dependencies are cached.")
		    .arg(readyCount)
		    .arg(trackedCount);
	}

	int CountReadyDependencies(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		int readyCount = 0;
		for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
		{
			if (DirectoryHasEntries(dependencyCachePath / dependency.CacheDirectoryName.toStdString()))
			{
				++readyCount;
			}
		}
		return readyCount;
	}

	QString DependencyGroupStatusText(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "Disabled";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return group.Required ? "Ready" : "Cached";
		}
		if (readyCount > 0)
		{
			return "Partial";
		}
		return group.Required ? "Pending sync" : "Available";
	}

	QString DependencyGroupStatusState(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "neutral";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return "ok";
		}
		return "warning";
	}

	QString FormatDependencyGroupDetail(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath, int readyCount)
	{
		Q_UNUSED(dependencyCachePath);
		QString detail = group.Summary + " " + group.UnlockSummary;
		if (!group.Enabled)
		{
			return detail + QStringLiteral(" Disabled by %1=OFF in this workspace configuration.").arg(group.ConfigureOption);
		}
		return detail + QStringLiteral(" %1 of %2 tracked dependencies are cached.")
		                    .arg(readyCount)
		                    .arg(group.Dependencies.size());
	}

	QString FormatDependencyEntryDetail(
	    const DependencyGroupUiEntry& group,
	    const ThirdPartyDependencyUiEntry& dependency,
	    const std::filesystem::path& dependencyPath)
	{
		QString detail = QStringLiteral("%1 Cache directory: %2 under the source dependency cache.")
		                     .arg(dependency.Purpose)
		                     .arg(QString::fromStdString(dependencyPath.filename().generic_string()));
		if (!group.Enabled)
		{
			detail += QStringLiteral(" Disabled by %1=OFF in this workspace configuration.").arg(group.ConfigureOption);
		}
		return detail;
	}

	bool OperationUsesDependencyGroup(const QString& operationId, const DependencyGroupUiEntry& group)
	{
		if (operationId == "workspace.setup")
		{
			return true;
		}
		if (group.Id == "core-workspace")
		{
			return operationId == "workspace.generate-solution" || operationId == "workspace.open-solution" || operationId == "workspace.build-all" ||
			    operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId.startsWith("cook.");
		}
		if (group.Id == "content-pipeline")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.textures" ||
			    operationId == "cook.assets" || operationId == "cook.project";
		}
		if (group.Id == "shader-compiler")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.shaders" ||
			    operationId == "cook.project";
		}
		if (group.Id == "ktx-support")
		{
			return operationId == "workspace.setup" || operationId == "cook.textures" || operationId == "cook.project";
		}
		return false;
	}
}
