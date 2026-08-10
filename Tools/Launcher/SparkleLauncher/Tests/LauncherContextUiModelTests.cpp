#include "LauncherContextUiModel.h"
#include "LauncherCapabilityProviders.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestMapping.h"
#include "LauncherSettings.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

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
	    ToolStatus("shader-compiler-sdk", ToolchainItemState::Found),
	    ToolStatus("visualstudio", ToolchainItemState::Found),
	    ToolStatus("visualstudio-ide", ToolchainItemState::Found),
	    ToolStatus("msbuild", ToolchainItemState::Found),
	    ToolStatus("clangcl", ToolchainItemState::Missing),
	    ToolStatus("rider", ToolchainItemState::Found),
	};

	const LauncherContextUiModel model = LauncherContextUiModel::Build(toolchain);
	BuildToolchainStatus missingShaderSdkToolchain = toolchain;
	for (ToolchainItemStatus& item : missingShaderSdkToolchain.Items)
	{
		if (item.Id == "shader-compiler-sdk")
		{
			item.State = ToolchainItemState::Missing;
		}
	}
	const LauncherContextUiModel missingShaderSdkModel = LauncherContextUiModel::Build(missingShaderSdkToolchain);
	LauncherSettings settings;
	LauncherOperationRequest buildRequest;
	buildRequest.BuildScopes = "editor;cook-tools;editor;unknown";
	const BuildWorkspaceOperationRequest mappedBuild = LauncherOperationRequestMapping::BuildWorkspace(buildRequest);
	LauncherOperationRequest cookRequest;
	cookRequest.CookScopes = "shaders;assets;shaders;unknown";
	const CookOperationRequest mappedCook = LauncherOperationRequestMapping::Cook(cookRequest);
	LauncherOperationRequest editorRequest;
	editorRequest.EditorProfile = "DevelopmentEditor";
	editorRequest.RuntimeProfile = "DevelopmentGame";
	const LauncherLevelUiModel levelModel;
	const LauncherCapabilityContext editorContext{editorRequest, levelModel};
	const LevelRunOperationRequest mappedEditor = LauncherOperationRequestMapping::LevelRun(editorRequest);

	LauncherOperationRequest gameRequest = editorRequest;
	gameRequest.RunMode = "game";
	const LauncherCapabilityContext gameContext{gameRequest, levelModel};
	const LevelRunOperationRequest mappedGame = LauncherOperationRequestMapping::LevelRun(gameRequest);
	std::string error;
	const bool runModesAreOrdered = model.RunModes.size() == 2 && model.RunModes[0].Value == "editor" && model.RunModes[1].Value == "game";
	const bool runModeContract = settings.RunMode() == "editor" && mappedEditor.RunMode == LevelRunMode::Editor
	    && mappedEditor.ProductProfile == "DevelopmentEditor" && editorContext.ProductBuildOperationId() == "workspace.build.editor"
	    && editorContext.ProductCapabilityId() == "product.editor" && mappedGame.RunMode == LevelRunMode::Game
	    && mappedGame.ProductProfile == "DevelopmentGame" && gameContext.ProductBuildOperationId() == "workspace.build.runtime"
	    && gameContext.ProductCapabilityId() == "product.runtime";
	const bool buildScopeContract = settings.BuildScopes() == "editor;runtime;cook-tools"
	    && mappedBuild.SelectedScopes == std::vector<BuildWorkspaceScope>{BuildWorkspaceScope::Editor, BuildWorkspaceScope::CookTools};
	const bool cookScopeContract = settings.CookScopes() == "shaders;textures;assets"
	    && mappedCook.SelectedScopes == std::vector<CookWorkspaceScope>{CookWorkspaceScope::Shaders, CookWorkspaceScope::SceneAssets};
	const bool valid = runModesAreOrdered && runModeContract && buildScopeContract && cookScopeContract
	    && ExpectAvailability(model.RunModes, "editor", true, error) && ExpectAvailability(model.RunModes, "game", true, error)
	    && ExpectAvailability(model.GraphicsApis, "d3d12", true, error) && ExpectAvailability(model.GraphicsApis, "vulkan", false, error)
	    && ExpectAvailability(model.ShaderBackends, "dxc", true, error) && ExpectAvailability(model.ShaderBackends, "slang", true, error)
	    && ExpectAvailability(missingShaderSdkModel.ShaderBackends, "dxc", true, error)
	    && ExpectAvailability(missingShaderSdkModel.ShaderBackends, "slang", true, error)
	    && ExpectAvailability(model.Compilers, "msvc", true, error) && ExpectAvailability(model.Compilers, "clang-cl", false, error)
	    && ExpectAvailability(model.Ides, "visual-studio", true, error) && ExpectAvailability(model.Ides, "rider", true, error)
	    && ExpectAvailability(model.BuildConfigurations, "development", true, error)
	    && ExpectAvailability(model.BuildConfigurations, "debug", true, error)
	    && ExpectAvailability(model.BuildConfigurations, "shipping", true, error);
	if (!valid)
	{
		if (!runModesAreOrdered)
		{
			error = "Run Mode must expose Editor first and Game second.";
		}
		else if (!runModeContract)
		{
			error = "Run Mode did not select the matching profile, build operation, and product capability.";
		}
		else if (!buildScopeContract)
		{
			error = "Build scope defaults or typed request mapping are incorrect.";
		}
		else if (!cookScopeContract)
		{
			error = "Cook scope defaults or typed request mapping are incorrect.";
		}
		std::cerr << error << '\n';
		return 1;
	}

	std::cout << "Launcher context selector availability passed." << '\n';
	return 0;
}
