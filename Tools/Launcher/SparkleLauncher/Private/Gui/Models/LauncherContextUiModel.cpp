#include "LauncherContextUiModel.h"

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QStringList>

#include <algorithm>
#include <iterator>
#include <string>

namespace SparkleLauncher
{
	static const ToolchainItemStatus* FindToolchainItem(const BuildToolchainStatus& toolchain, const std::string& id)
	{
		const auto found =
		    std::find_if(toolchain.Items.begin(), toolchain.Items.end(), [&id](const ToolchainItemStatus& item) { return item.Id == id; });
		return found == toolchain.Items.end() ? nullptr : &*found;
	}

	static bool IsToolAvailable(const BuildToolchainStatus& toolchain, const std::string& id)
	{
		const ToolchainItemStatus* item = FindToolchainItem(toolchain, id);
		return item != nullptr && item->State == ToolchainItemState::Found;
	}

	static QString PathDetail(const std::filesystem::path& path, const QString& fallback)
	{
		return path.empty() ? fallback : QString::fromStdString(path.string());
	}

	static QVector<LauncherSelectionOption> BuildGraphicsApiOptions(const BuildToolchainStatus& toolchain)
	{
		const bool d3d12Available = IsToolAvailable(toolchain, "windowssdk");
		const bool vulkanAvailable = IsToolAvailable(toolchain, "vulkan-sdk");
		return {
		    {"D3D12",
		        "d3d12",
		        d3d12Available ? QStringLiteral("Windows SDK %1 is installed.").arg(QString::fromStdString(toolchain.WindowsSdkVersion))
		                       : QStringLiteral("Install the Windows SDK through Visual Studio Installer, then reactivate the launcher."),
		        d3d12Available},
		    {"Vulkan",
		        "vulkan",
		        vulkanAvailable ? PathDetail(toolchain.VulkanSdkRoot, QStringLiteral("The Vulkan SDK is installed."))
		                        : QStringLiteral("Install the Vulkan SDK, then reactivate the launcher."),
		        vulkanAvailable},
		};
	}

	struct BuildConfigurationSupport final
	{
		QString DisplayName;
		QString Value;
		bool HasEditorProfile = false;
		bool HasRuntimeProfile = false;
	};

	static QVector<LauncherSelectionOption> BuildConfigurationOptions()
	{
		QVector<BuildConfigurationSupport> supportedConfigurations;
		for (const BuildProfile& profile : GetBuildProfileCatalog())
		{
			const QString displayName = QString::fromStdString(ToString(profile.State));
			const QString value = displayName.toLower();
			auto found = std::find_if(
			    supportedConfigurations.begin(),
			    supportedConfigurations.end(),
			    [&value](const BuildConfigurationSupport& configuration) { return configuration.Value == value; });
			if (found == supportedConfigurations.end())
			{
				supportedConfigurations.push_back({displayName, value});
				found = std::prev(supportedConfigurations.end());
			}

			found->HasEditorProfile = found->HasEditorProfile || profile.Target == BuildProfileTarget::Editor;
			found->HasRuntimeProfile = found->HasRuntimeProfile || profile.Target == BuildProfileTarget::Game;
		}

		QVector<LauncherSelectionOption> options;
		options.reserve(supportedConfigurations.size());
		for (const BuildConfigurationSupport& configuration : supportedConfigurations)
		{
			const bool available = configuration.HasEditorProfile && configuration.HasRuntimeProfile;
			QStringList missingProfiles;
			if (!configuration.HasEditorProfile)
			{
				missingProfiles.push_back("editor");
			}
			if (!configuration.HasRuntimeProfile)
			{
				missingProfiles.push_back("runtime");
			}
			options.push_back(
			    {configuration.DisplayName,
			        configuration.Value,
			        available ? QStringLiteral("Editor and runtime profiles can be prepared automatically.")
			                  : QStringLiteral("Missing %1 build profile support.").arg(missingProfiles.join(" and ")),
			        available});
		}
		return options;
	}

	static QVector<LauncherSelectionOption> BuildCompilerOptions(const BuildToolchainStatus& toolchain)
	{
		const bool visualStudioAvailable = IsToolAvailable(toolchain, "visualstudio");
		const bool msvcAvailable = visualStudioAvailable && IsToolAvailable(toolchain, "msbuild");
		const ToolchainItemStatus* clangCl = FindToolchainItem(toolchain, "clangcl");
		const bool clangClAvailable = visualStudioAvailable && clangCl != nullptr && clangCl->State == ToolchainItemState::Found;
		return {
		    {"MSVC",
		        "msvc",
		        msvcAvailable ? PathDetail(toolchain.VisualStudioPath, QStringLiteral("Visual Studio C++ tools are installed."))
		                      : QStringLiteral("Install Visual Studio C++ tools and MSBuild, then reactivate the launcher."),
		        msvcAvailable},
		    {"clang-cl",
		        "clang-cl",
		        clangClAvailable ? PathDetail(toolchain.ClangClPath, QStringLiteral("clang-cl is installed."))
		                         : (clangCl != nullptr && clangCl->CanInstall
		                                   ? QStringLiteral("Open Sync > Sync Code and choose Install for clang-cl.")
		                                   : QStringLiteral("Install the Visual Studio clang-cl component, then reactivate the launcher.")),
		        clangClAvailable},
		};
	}

	static QVector<LauncherSelectionOption> BuildIdeOptions(const BuildToolchainStatus& toolchain)
	{
		const bool visualStudioAvailable = IsToolAvailable(toolchain, "visualstudio-ide");
		const bool riderAvailable = IsToolAvailable(toolchain, "rider");
		return {
		    {"Visual Studio",
		        "visual-studio",
		        visualStudioAvailable ? PathDetail(toolchain.VisualStudioIdePath, QStringLiteral("Visual Studio is installed."))
		                              : QStringLiteral("Install Visual Studio with C++ tools, then reactivate the launcher."),
		        visualStudioAvailable},
		    {"Rider",
		        "rider",
		        riderAvailable ? PathDetail(toolchain.RiderPath, QStringLiteral("Rider is installed."))
		                       : QStringLiteral("Install Rider, then reactivate the launcher."),
		        riderAvailable},
		};
	}

	LauncherContextUiModel LauncherContextUiModel::Build(const BuildToolchainStatus& toolchain)
	{
		return {
		    .GraphicsApis = BuildGraphicsApiOptions(toolchain),
		    .BuildConfigurations = BuildConfigurationOptions(),
		    .Compilers = BuildCompilerOptions(toolchain),
		    .Ides = BuildIdeOptions(toolchain),
		};
	}
}
