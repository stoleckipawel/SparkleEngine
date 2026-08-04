#include "LauncherContextUiModel.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>

#include <iostream>
#include <string>
#include <utility>

namespace SparkleLauncher
{
	static ToolchainItemStatus ToolStatus(std::string id, ToolchainItemState state)
	{
		ToolchainItemStatus status;
		status.Id = std::move(id);
		status.State = state;
		return status;
	}

	static const LauncherSelectionOption* FindOption(const QVector<LauncherSelectionOption>& options, const QString& value)
	{
		for (const LauncherSelectionOption& option : options)
		{
			if (option.Value == value)
			{
				return &option;
			}
		}
		return nullptr;
	}

	static bool ExpectAvailability(const QVector<LauncherSelectionOption>& options, const QString& value, bool expected, std::string& error)
	{
		const LauncherSelectionOption* option = FindOption(options, value);
		if (option == nullptr)
		{
			error = "Missing supported selector option: " + value.toStdString();
			return false;
		}
		if (option->Available != expected)
		{
			error = "Incorrect availability for selector option: " + value.toStdString();
			return false;
		}
		if (option->Detail.isEmpty())
		{
			error = "Selector option does not explain its availability: " + value.toStdString();
			return false;
		}
		return true;
	}
}

int main()
{
	using namespace SparkleLauncher;

	BuildToolchainStatus toolchain;
	toolchain.WindowsSdkVersion = "10.0.26100.0";
	toolchain.VisualStudioPath = "C:/VisualStudio";
	toolchain.VisualStudioIdePath = "C:/VisualStudio/Common7/IDE/devenv.exe";
	toolchain.RiderPath = "C:/Rider/bin/rider64.exe";
	toolchain.Items = {
	    ToolStatus("windowssdk", ToolchainItemState::Found),
	    ToolStatus("vulkan-sdk", ToolchainItemState::Missing),
	    ToolStatus("visualstudio", ToolchainItemState::Found),
	    ToolStatus("visualstudio-ide", ToolchainItemState::Found),
	    ToolStatus("msbuild", ToolchainItemState::Found),
	    ToolStatus("clangcl", ToolchainItemState::Missing),
	    ToolStatus("rider", ToolchainItemState::Found),
	};

	const LauncherContextUiModel model = LauncherContextUiModel::Build(toolchain);
	std::string error;
	const bool valid = ExpectAvailability(model.GraphicsApis, "d3d12", true, error)
	    && ExpectAvailability(model.GraphicsApis, "vulkan", false, error) && ExpectAvailability(model.Compilers, "msvc", true, error)
	    && ExpectAvailability(model.Compilers, "clang-cl", false, error) && ExpectAvailability(model.Ides, "visual-studio", true, error)
	    && ExpectAvailability(model.Ides, "rider", true, error) && ExpectAvailability(model.BuildConfigurations, "development", true, error)
	    && ExpectAvailability(model.BuildConfigurations, "debug", true, error)
	    && ExpectAvailability(model.BuildConfigurations, "shipping", true, error);
	if (!valid)
	{
		std::cerr << error << '\n';
		return 1;
	}

	std::cout << "Launcher context selector availability passed." << '\n';
	return 0;
}
