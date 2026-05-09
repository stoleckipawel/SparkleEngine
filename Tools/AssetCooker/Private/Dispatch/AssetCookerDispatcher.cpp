#include "AssetCookerDispatcher.h"

#include "AssetCookerToolProcess.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <system_error>
#include <utility>

static bool AssetCookerFileExists(const std::filesystem::path& path)
{
	std::error_code errorCode;
	return std::filesystem::exists(path, errorCode);
}

static std::filesystem::path AssetCookerResolveToolPath(
    const AssetCookerProjectCookPlan& plan,
    std::string_view executableName)
{
	const std::string fileName = std::string(executableName) + ".exe";
	const std::filesystem::path configuredPath = plan.repositoryRoot / "build" / "bin" / plan.configuration / fileName;
	if (AssetCookerFileExists(configuredPath))
	{
		return configuredPath;
	}

	return plan.repositoryRoot / "build" / "bin" / fileName;
}

static bool AssetCookerPlanUsesStep(const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step)
{
	for (const AssetCookerPlanStep planStep : plan.steps)
	{
		if (planStep == step)
		{
			return true;
		}
	}
	return false;
}

static std::filesystem::path AssetCookerMakeTempPath(
    const AssetCookerProjectCookPlan& plan,
    std::string_view stem,
    std::string_view extension)
{
	const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
	return plan.repositoryRoot / "build" / "Cook" / "Temp" /
	       (std::string(stem) + "-" + plan.projectName + "-" + std::to_string(timestamp) + std::string(extension));
}

static bool AssetCookerWriteSceneList(
    const AssetCookerProjectCookPlan& plan,
    const std::filesystem::path& sceneListPath,
    AssetCookerDiagnostics& diagnostics)
{
	std::error_code createError;
	std::filesystem::create_directories(sceneListPath.parent_path(), createError);
	if (createError)
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, "Failed to create scene-list temp directory.", sceneListPath.parent_path());
		return false;
	}

	std::ofstream output(sceneListPath, std::ios::binary);
	if (!output.is_open())
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, "Failed to write scene-list temp file.", sceneListPath);
		return false;
	}

	for (const AssetCookerSceneEntry& sceneEntry : plan.sceneEntries)
	{
		output << sceneEntry.origin << "|" << sceneEntry.relativePath << "|" << sceneEntry.sourcePath.string() << "\n";
	}

	return true;
}

static void AssetCookerRemoveTempFile(const std::filesystem::path& path)
{
	std::error_code removeError;
	std::filesystem::remove(path, removeError);
}

static bool AssetCookerRunShaders(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
	int exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"list-shaders", L"--validate"}, plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader registration validation failed.");
		return false;
	}

	exitCode = AssetCookerToolProcess::Run(shaderCompilerPath, {L"cook"}, plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader package cooking failed.");
		return false;
	}

	AssetCookerOutputRecord packagesOutput;
	packagesOutput.category = AssetCookerCategory_Shaders;
	packagesOutput.assetId = "shader-packages";
	packagesOutput.path = (plan.cookedRoot / "Shaders" / "Packages").string();
	packagesOutput.reloadHint = "Reload shader packages and affected pipelines.";
	outOutputs.push_back(std::move(packagesOutput));

	AssetCookerOutputRecord registryOutput;
	registryOutput.category = AssetCookerCategory_Shaders;
	registryOutput.assetId = "shader-registry";
	registryOutput.path = (plan.cookedRoot / "Shaders" / "ShaderPackageRegistry.sreg").string();
	registryOutput.reloadHint = "Reload shader package registry.";
	outOutputs.push_back(std::move(registryOutput));
	return true;
}

static bool AssetCookerRunTextures(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path assetConverterPath = AssetCookerResolveToolPath(plan, "AssetConverter");
	const std::filesystem::path textureCookerPath = AssetCookerResolveToolPath(plan, "TextureCooker");
	const std::filesystem::path sceneListPath = AssetCookerMakeTempPath(plan, "assetcooker-scenes", ".txt");
	const std::filesystem::path textureRequestPath = AssetCookerMakeTempPath(plan, "assetcooker-texture-requests", ".txt");

	if (!AssetCookerWriteSceneList(plan, sceneListPath, diagnostics))
	{
		return false;
	}

	const std::wstring totalSceneCount = std::to_wstring(plan.sceneEntries.size());
	int exitCode = AssetCookerToolProcess::Run(
	    assetConverterPath,
	    {L"collect-texture-request-list", sceneListPath.wstring(), totalSceneCount, textureRequestPath.wstring()},
	    plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, "Texture request collection failed.");
		AssetCookerRemoveTempFile(sceneListPath);
		AssetCookerRemoveTempFile(textureRequestPath);
		return false;
	}

	exitCode = AssetCookerToolProcess::Run(textureCookerPath, {L"cook-request-file", textureRequestPath.wstring()}, plan.projectRoot);
	AssetCookerRemoveTempFile(sceneListPath);
	AssetCookerRemoveTempFile(textureRequestPath);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, "Texture asset cooking failed.");
		return false;
	}

	AssetCookerOutputRecord output;
	output.category = AssetCookerCategory_Textures;
	output.assetId = "textures";
	output.path = (plan.cookedRoot / "Textures").string();
	output.reloadHint = "Reload changed cooked textures.";
	outOutputs.push_back(std::move(output));
	return true;
}

static bool AssetCookerRunSceneAssets(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	const std::filesystem::path assetConverterPath = AssetCookerResolveToolPath(plan, "AssetConverter");
	const std::filesystem::path sceneListPath = AssetCookerMakeTempPath(plan, "assetcooker-scenes", ".txt");
	if (!AssetCookerWriteSceneList(plan, sceneListPath, diagnostics))
	{
		return false;
	}

	const std::wstring totalSceneCount = std::to_wstring(plan.sceneEntries.size());
	const int exitCode = AssetCookerToolProcess::Run(
	    assetConverterPath,
	    {L"cook-scene-list", sceneListPath.wstring(), totalSceneCount},
	    plan.projectRoot);
	AssetCookerRemoveTempFile(sceneListPath);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_SceneAssets, "Scene, mesh, and material asset cooking failed.");
		return false;
	}

	AssetCookerOutputRecord sceneOutput;
	sceneOutput.category = AssetCookerCategory_SceneAssets;
	sceneOutput.assetId = "scene-manifests";
	sceneOutput.path = (plan.cookedRoot / "SceneManifests").string();
	sceneOutput.reloadHint = "Reload changed scenes and referenced cooked assets.";
	outOutputs.push_back(std::move(sceneOutput));

	AssetCookerOutputRecord meshOutput;
	meshOutput.category = AssetCookerCategory_Mesh;
	meshOutput.assetId = "meshes";
	meshOutput.path = (plan.cookedRoot / "Meshes").string();
	meshOutput.reloadHint = "Reload changed cooked meshes.";
	outOutputs.push_back(std::move(meshOutput));

	AssetCookerOutputRecord materialOutput;
	materialOutput.category = AssetCookerCategory_Material;
	materialOutput.assetId = "materials";
	materialOutput.path = (plan.cookedRoot / "Materials").string();
	materialOutput.reloadHint = "Reload changed cooked materials.";
	outOutputs.push_back(std::move(materialOutput));
	return true;
}

bool AssetCookerDispatcher::ValidateCapabilities(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics)
{
	bool result = true;
	if (AssetCookerPlanUsesStep(plan, AssetCookerPlanStep::Shaders))
	{
		const std::filesystem::path shaderCompilerPath = AssetCookerResolveToolPath(plan, "ShaderCompiler");
		if (!AssetCookerFileExists(shaderCompilerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "ShaderCompiler executable was not found.", shaderCompilerPath);
			result = false;
		}
	}

	if (AssetCookerPlanUsesStep(plan, AssetCookerPlanStep::Textures))
	{
		const std::filesystem::path assetConverterPath = AssetCookerResolveToolPath(plan, "AssetConverter");
		const std::filesystem::path textureCookerPath = AssetCookerResolveToolPath(plan, "TextureCooker");
		if (!AssetCookerFileExists(assetConverterPath))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, "AssetConverter executable was not found.", assetConverterPath);
			result = false;
		}
		if (!AssetCookerFileExists(textureCookerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, "TextureCooker executable was not found.", textureCookerPath);
			result = false;
		}
	}

	if (AssetCookerPlanUsesStep(plan, AssetCookerPlanStep::SceneAssets))
	{
		const std::filesystem::path assetConverterPath = AssetCookerResolveToolPath(plan, "AssetConverter");
		if (!AssetCookerFileExists(assetConverterPath))
		{
			diagnostics.AddError(AssetCookerCategory_SceneAssets, "AssetConverter executable was not found.", assetConverterPath);
			result = false;
		}
	}

	return result;
}

bool AssetCookerDispatcher::DispatchPlan(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	std::cout << "AssetCooker Plan:\n"
	          << "  project=" << plan.projectName << "\n"
	          << "  configuration=" << plan.configuration << "\n"
	          << "  engineScenes=" << plan.engineSceneCount << "\n"
	          << "  projectScenes=" << plan.projectSceneCount << "\n"
	          << "  overrides=" << plan.overriddenEngineSceneCount << "\n"
	          << "  finalScenes=" << plan.sceneEntries.size() << "\n";

	for (const AssetCookerPlanStep step : plan.steps)
	{
		if (step == AssetCookerPlanStep::Shaders && !AssetCookerRunShaders(plan, diagnostics, outOutputs))
		{
			return false;
		}

		if (step == AssetCookerPlanStep::Textures && !AssetCookerRunTextures(plan, diagnostics, outOutputs))
		{
			return false;
		}

		if (step == AssetCookerPlanStep::SceneAssets && !AssetCookerRunSceneAssets(plan, diagnostics, outOutputs))
		{
			return false;
		}
	}

	return true;
}
