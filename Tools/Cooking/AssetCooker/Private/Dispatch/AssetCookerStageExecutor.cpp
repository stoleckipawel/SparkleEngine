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

class AssetCookerStageExecutorImplementation final
{
  public:
	static bool FileExists(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::exists(path, errorCode);
	}

	static std::filesystem::path ResolveToolPath(
	    const AssetCookerProjectCookPlan& plan, std::string_view executableName)
	{
		const std::string fileName = std::string(executableName) + ".exe";
		const std::filesystem::path artifactPath =
		    plan.repositoryRoot / "artifacts" / "dev" / "tools" / executableName / plan.toolConfiguration / fileName;
		return FileExists(artifactPath) ? artifactPath :
		                                  plan.repositoryRoot / "build" / "bin" / plan.toolConfiguration / fileName;
	}

	static std::filesystem::path MakeTemporaryPath(
	    const AssetCookerProjectCookPlan& plan, std::string_view stem, std::string_view extension)
	{
		const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
		return plan.repositoryRoot / "artifacts" / "dev" / "tools" / "AssetCooker" / "Temp" /
		       (std::string(stem) + "-" + plan.projectName + "-" + std::to_string(timestamp) + std::string(extension));
	}

	class ScopedTemporaryFile final
	{
	  public:
		explicit ScopedTemporaryFile(std::filesystem::path path) : m_path(std::move(path)) {}
		~ScopedTemporaryFile()
		{
			std::error_code errorCode;
			std::filesystem::remove(m_path, errorCode);
		}

		const std::filesystem::path& GetPath() const noexcept { return m_path; }

		ScopedTemporaryFile(const ScopedTemporaryFile&) = delete;
		ScopedTemporaryFile& operator=(const ScopedTemporaryFile&) = delete;

	  private:
		std::filesystem::path m_path;
	};

	static void AppendOutput(
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

	static bool RunShaders(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		if (AssetCookerToolProcess::Run(ResolveToolPath(plan, "ShaderCompiler"), {"cook"}, plan.projectRoot) != 0)
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "Shader package cooking failed.");
			return false;
		}

		AppendOutput(
		    outputs,
		    AssetCookerCategory_Shaders,
		    "shader-packages",
		    plan.cookedRoot / "Shaders" / "Packages");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Shaders,
		    "shader-registry",
		    plan.cookedRoot / "Shaders" / "ShaderPackageRegistry.sreg");
		return true;
	}

	static bool RunTextures(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		const ScopedTemporaryFile requestFile(MakeTemporaryPath(plan, "assetcooker-texture-requests", ".txt"));

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

		AppendOutput(
		    outputs,
		    AssetCookerCategory_Textures,
		    "textures",
		    plan.cookedRoot / "Textures");
		return true;
	}

	static bool RunSceneAssets(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		if (!AssetCookerSceneBatch::Execute(plan.sceneEntries, diagnostics))
		{
			return false;
		}

		AppendOutput(
		    outputs,
		    AssetCookerCategory_SceneAssets,
		    "scene-manifests",
		    plan.cookedRoot / "SceneManifests");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Meshes,
		    "meshes",
		    plan.cookedRoot / "Meshes");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Materials,
		    "materials",
		    plan.cookedRoot / "Materials");
		return true;
	}
};

const char* AssetCookerStageExecutor::GetStepName(AssetCookerPlanStep step) noexcept
{
	switch (step)
	{
		case AssetCookerPlanStep::Shaders: return "shaders";
		case AssetCookerPlanStep::Textures: return "textures";
		case AssetCookerPlanStep::SceneAssets: return "scene-assets";
		default: return "unknown";
	}
}

bool AssetCookerStageExecutor::PlanUsesStep(
    const AssetCookerProjectCookPlan& plan, AssetCookerPlanStep step) noexcept
{
	return std::ranges::find(plan.steps, step) != plan.steps.end();
}

bool AssetCookerStageExecutor::ValidateCapabilities(
    const AssetCookerProjectCookPlan& plan, AssetCookerDiagnostics& diagnostics)
{
	bool valid = true;
	if (PlanUsesStep(plan, AssetCookerPlanStep::Shaders))
	{
		const std::filesystem::path compilerPath = AssetCookerStageExecutorImplementation::ResolveToolPath(plan, "ShaderCompiler");
		if (!AssetCookerStageExecutorImplementation::FileExists(compilerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "ShaderCompiler executable was not found.", compilerPath);
			valid = false;
		}
	}
	if (PlanUsesStep(plan, AssetCookerPlanStep::Textures))
	{
		const std::filesystem::path cookerPath = AssetCookerStageExecutorImplementation::ResolveToolPath(plan, "TextureCooker");
		if (!AssetCookerStageExecutorImplementation::FileExists(cookerPath))
		{
			diagnostics.AddError(AssetCookerCategory_Textures, "TextureCooker executable was not found.", cookerPath);
			valid = false;
		}
	}
	return valid;
}

bool AssetCookerStageExecutor::Execute(
    AssetCookerPlanStep step,
    const AssetCookerProjectCookPlan& plan,
    AssetCookerDiagnostics& diagnostics,
    std::vector<AssetCookerOutputRecord>& outOutputs)
{
	switch (step)
	{
		case AssetCookerPlanStep::Shaders: return AssetCookerStageExecutorImplementation::RunShaders(plan, diagnostics, outOutputs);
		case AssetCookerPlanStep::Textures: return AssetCookerStageExecutorImplementation::RunTextures(plan, diagnostics, outOutputs);
		case AssetCookerPlanStep::SceneAssets: return AssetCookerStageExecutorImplementation::RunSceneAssets(plan, diagnostics, outOutputs);
		default: return false;
	}
}
