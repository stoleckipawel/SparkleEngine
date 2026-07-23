#include "AssetCookerStageExecutor.h"

#include "AssetCookerToolProcess.h"
#include "Cooking/ImportedSceneCooker.h"
#include "Cooking/TextureRequestPlanBuilder.h"
#include "SourceSceneImporter.h"
#include "ToolConsole.h"

#include <algorithm>
#include <chrono>
#include <iostream>
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
	    std::filesystem::path path,
	    std::string reloadHint)
	{
		AssetCookerOutputRecord output;
		output.category = category;
		output.assetId = std::move(assetId);
		output.path = path.string();
		output.reloadHint = std::move(reloadHint);
		outputs.push_back(std::move(output));
	}

	static bool RunShaders(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		ToolConsole::Info("Cooking shaders: writing package payloads...");
		if (AssetCookerToolProcess::Run(ResolveToolPath(plan, "ShaderCompiler"), {"cook"}, plan.projectRoot) != 0)
		{
			diagnostics.AddError(AssetCookerCategory_Shaders, "Shader package cooking failed.");
			return false;
		}

		AppendOutput(
		    outputs,
		    AssetCookerCategory_Shaders,
		    "shader-packages",
		    plan.cookedRoot / "Shaders" / "Packages",
		    "Reload shader packages and affected pipelines.");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Shaders,
		    "shader-registry",
		    plan.cookedRoot / "Shaders" / "ShaderPackageRegistry.sreg",
		    "Reload shader package registry.");
		return true;
	}

	static bool RunTextures(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		const ScopedTemporaryFile requestFile(MakeTemporaryPath(plan, "assetcooker-texture-requests", ".txt"));
		ToolConsole::Info("Cooking textures: building request plan from scene materials...");

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
		    plan.cookedRoot / "Textures",
		    "Reload changed cooked textures.");
		return true;
	}

	static bool RunSceneAssets(
	    const AssetCookerProjectCookPlan& plan,
	    AssetCookerDiagnostics& diagnostics,
	    std::vector<AssetCookerOutputRecord>& outputs)
	{
		std::size_t failedSceneCount = 0;
		std::size_t cookedSceneCount = 0;
		for (const AssetCookerSceneEntry& sceneEntry : plan.sceneEntries)
		{
			ToolConsole::Progress(
			    std::cout,
			    "Cooking",
			    "scene",
			    cookedSceneCount + 1u,
			    plan.sceneEntries.size(),
			    sceneEntry.relativePath,
			    {ToolConsole::Field("origin", sceneEntry.origin)});

			const bool cooked = ImportedSceneCooker::ImportAndVisit(
			    sceneEntry,
			    AssetCookerCategory_SceneAssets,
			    diagnostics,
			    [&](const SourceImportResult& importResult)
			    {
				    return ImportedSceneCooker::Cook(sceneEntry, importResult, diagnostics);
			    });
			if (!cooked)
			{
				++failedSceneCount;
				continue;
			}
			++cookedSceneCount;
		}

		if (failedSceneCount != 0)
		{
			diagnostics.AddError(
			    AssetCookerCategory_SceneAssets,
			    "Scene, mesh, and material asset cooking failed for " + std::to_string(failedSceneCount) + " scene(s).");
			return false;
		}

		AppendOutput(
		    outputs,
		    AssetCookerCategory_SceneAssets,
		    "scene-manifests",
		    plan.cookedRoot / "SceneManifests",
		    "Reload changed scenes and referenced cooked assets.");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Mesh,
		    "meshes",
		    plan.cookedRoot / "Meshes",
		    "Reload changed cooked meshes.");
		AppendOutput(
		    outputs,
		    AssetCookerCategory_Material,
		    "materials",
		    plan.cookedRoot / "Materials",
		    "Reload changed cooked materials.");
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
