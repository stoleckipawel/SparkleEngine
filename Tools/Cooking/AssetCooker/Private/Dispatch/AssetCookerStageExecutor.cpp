#include "AssetCookerStageExecutor.h"

#include "AssetCookerToolProcess.h"
#include "Cooking/AssetCookerSceneBatch.h"
#include "Cooking/TextureRequestPlanBuilder.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

class AssetCookerTemporaryFile final
{
public:
	explicit AssetCookerTemporaryFile(std::filesystem::path path);
	~AssetCookerTemporaryFile();

	const std::filesystem::path& GetPath() const noexcept;

	AssetCookerTemporaryFile(const AssetCookerTemporaryFile&) = delete;
	AssetCookerTemporaryFile& operator=(const AssetCookerTemporaryFile&) = delete;

private:
	std::filesystem::path m_path;
};

AssetCookerTemporaryFile::AssetCookerTemporaryFile(std::filesystem::path path) :
    m_path(std::move(path))
{
}

AssetCookerTemporaryFile::~AssetCookerTemporaryFile()
{
	std::error_code errorCode;
	std::filesystem::remove(m_path, errorCode);
}

const std::filesystem::path& AssetCookerTemporaryFile::GetPath() const noexcept
{
	return m_path;
}

const char* AssetCookerStageExecutor::GetStepName(AssetCookerPlanStep step) noexcept
{
	switch (step)
	{
		case AssetCookerPlanStep::Shaders:
			return "shaders";
		case AssetCookerPlanStep::Textures:
			return "textures";
		case AssetCookerPlanStep::SceneAssets:
			return "scene-assets";
		default:
			return "unknown";
	}
}

bool AssetCookerStageExecutor::FileExists(const std::filesystem::path& path)
{
	std::error_code errorCode;
	return std::filesystem::exists(path, errorCode);
}

bool AssetCookerStageExecutor::PlanUsesStep(const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step) noexcept
{
	return std::ranges::find(plan.steps, step) != plan.steps.end();
}

std::filesystem::path AssetCookerStageExecutor::ResolveToolPath(const AssetCookerProjectCookPlan& plan, std::string_view executableName)
{
	return plan.repositoryRoot / "artifacts" / "dev" / "tools" / executableName / plan.toolProfile / (std::string(executableName) + ".exe");
}

std::filesystem::path AssetCookerStageExecutor::MakeTemporaryPath(
    const AssetCookerProjectCookPlan& plan,
    std::string_view stem,
    std::string_view extension)
{
	const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
	return plan.repositoryRoot / "artifacts" / "dev" / "tools" / "AssetCooker" / "Temp"
	    / (std::string(stem) + "-" + plan.projectName + "-" + std::to_string(timestamp) + std::string(extension));
}

void AssetCookerStageExecutor::AppendOutput(
    std::vector<AssetCookerOutputRecord>& outputs,
    AssetCookerCategory category,
    std::string assetId,
    std::filesystem::path path)
{
	AssetCookerOutputRecord output;
	output.category = category;
	output.assetId = std::move(assetId);
	output.path = path.string();
	outputs.push_back(std::move(output));
}

bool AssetCookerStageExecutor::ValidateCapabilities(const AssetCookerProjectCookPlan& plan, AssetCookerDiagnostics& diagnostics)
{
	bool valid = true;
	if (PlanUsesStep(plan, AssetCookerPlanStep::Shaders))
	{
		const std::filesystem::path compilerPath = ResolveToolPath(plan, "ShaderCompiler");
		if (!FileExists(compilerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "ShaderCompiler executable was not found.", compilerPath);
			valid = false;
		}
	}
	if (PlanUsesStep(plan, AssetCookerPlanStep::Textures))
	{
		const std::filesystem::path cookerPath = ResolveToolPath(plan, "TextureCooker");
		if (!FileExists(cookerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, "TextureCooker executable was not found.", cookerPath);
			valid = false;
		}
	}
	return valid;
}

bool AssetCookerStageExecutor::RunShaders(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outputs)
{
	if (AssetCookerToolProcess::Run(ResolveToolPath(plan, "ShaderCompiler"), {"cook"}, plan.projectRoot) != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Shaders, "Shader cooking failed.");
		return false;
	}

	AppendOutput(outputs, AssetCookerCategory_Shaders, "global-shader-map", plan.cookedRoot / "Shaders" / "GlobalShaderMap.smap");
	AppendOutput(outputs, AssetCookerCategory_Shaders, "cooked-shader-library", plan.cookedRoot / "Shaders" / "CookedShaderLibrary.slib");
	return true;
}

bool AssetCookerStageExecutor::RunTextures(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outputs)
{
	const AssetCookerTemporaryFile requestFile(MakeTemporaryPath(plan, "assetcooker-texture-requests", ".txt"));

	std::error_code errorCode;
	std::filesystem::create_directories(requestFile.GetPath().parent_path(), errorCode);
	if (errorCode)
	{
		diagnostics.AddError(
		    AssetCookerCategory_Textures,
		    "Failed to create texture-request temp directory.",
		    requestFile.GetPath().parent_path());
		return false;
	}

	if (!TextureRequestPlanBuilder::Build(plan, diagnostics, requestFile.GetPath()))
	{
		return false;
	}

	const int exitCode = AssetCookerToolProcess::Run(
	    ResolveToolPath(plan, "TextureCooker"),
	    {"cook-request-file", requestFile.GetPath().string()},
	    plan.projectRoot);
	if (exitCode != 0)
	{
		diagnostics.AddError(AssetCookerCategory_Textures, "Texture asset cooking failed.");
		return false;
	}

	AppendOutput(outputs, AssetCookerCategory_Textures, "textures", plan.cookedRoot / "Textures");
	return true;
}

bool AssetCookerStageExecutor::RunSceneAssets(
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outputs)
{
	if (!AssetCookerSceneBatch::Execute(plan.sceneEntries, diagnostics))
	{
		return false;
	}

	AppendOutput(outputs, AssetCookerCategory_SceneAssets, "scene-manifests", plan.cookedRoot / "SceneManifests");
	AppendOutput(outputs, AssetCookerCategory_Meshes, "meshes", plan.cookedRoot / "Meshes");
	AppendOutput(outputs, AssetCookerCategory_Materials, "materials", plan.cookedRoot / "Materials");
	return true;
}

bool AssetCookerStageExecutor::Execute(
    AssetCookerPlanStep step,
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	switch (step)
	{
		case AssetCookerPlanStep::Shaders:
			return RunShaders(plan, diagnostics, outOutputs);
		case AssetCookerPlanStep::Textures:
			return RunTextures(plan, diagnostics, outOutputs);
		case AssetCookerPlanStep::SceneAssets:
			return RunSceneAssets(plan, diagnostics, outOutputs);
		default:
			return false;
	}
}
