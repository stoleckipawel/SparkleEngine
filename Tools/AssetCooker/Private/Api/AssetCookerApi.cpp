#include "../../Public/AssetCookerApi.h"

#include "AssetCookerService.h"

#include <memory>
#include <vector>

struct AssetCookerContext final
{
	explicit AssetCookerContext(const AssetCookerConfig* config)
	    : service(config)
	{
	}

	AssetCookerService service;
	AssetCookerServiceResult lastServiceResult;
	std::vector<AssetCookDiagnostic> lastDiagnosticViews;
	std::vector<AssetCookedOutput> lastOutputViews;
	AssetCookResult lastPublicResult = {};
};

static void AssetCookerBuildPublicResult(AssetCookerContext* context, AssetCookResult* outResult)
{
	context->lastDiagnosticViews.clear();
	context->lastDiagnosticViews.reserve(context->lastServiceResult.diagnostics.size());
	for (const AssetCookerDiagnosticRecord& diagnostic : context->lastServiceResult.diagnostics)
	{
		AssetCookDiagnostic publicDiagnostic;
		publicDiagnostic.severity = diagnostic.severity;
		publicDiagnostic.category = diagnostic.category;
		publicDiagnostic.message = diagnostic.message.c_str();
		publicDiagnostic.sourcePath = diagnostic.sourcePath.c_str();
		context->lastDiagnosticViews.push_back(publicDiagnostic);
	}

	context->lastOutputViews.clear();
	context->lastOutputViews.reserve(context->lastServiceResult.outputs.size());
	for (const AssetCookerOutputRecord& output : context->lastServiceResult.outputs)
	{
		AssetCookedOutput publicOutput;
		publicOutput.category = output.category;
		publicOutput.assetId = output.assetId.c_str();
		publicOutput.path = output.path.c_str();
		publicOutput.reloadHint = output.reloadHint.c_str();
		publicOutput.version = output.version;
		context->lastOutputViews.push_back(publicOutput);
	}

	context->lastPublicResult.succeeded = context->lastServiceResult.succeeded ? 1U : 0U;
	context->lastPublicResult.exitCode = context->lastServiceResult.exitCode;
	context->lastPublicResult.outputs = context->lastOutputViews.empty() ? nullptr : context->lastOutputViews.data();
	context->lastPublicResult.outputCount = static_cast<std::uint32_t>(context->lastOutputViews.size());
	context->lastPublicResult.diagnostics = context->lastDiagnosticViews.empty() ? nullptr : context->lastDiagnosticViews.data();
	context->lastPublicResult.diagnosticCount = static_cast<std::uint32_t>(context->lastDiagnosticViews.size());

	if (outResult != nullptr)
	{
		*outResult = context->lastPublicResult;
	}
}

AssetCookerContext* AssetCookerCreateContext(const AssetCookerConfig* config)
{
	return new AssetCookerContext(config);
}

void AssetCookerDestroyContext(AssetCookerContext* context)
{
	delete context;
}

int AssetCookerCookProject(
    AssetCookerContext* context,
    const AssetCookRequest* request,
    AssetCookResult* outResult)
{
	if (context == nullptr)
	{
		return 1;
	}

	context->lastServiceResult = context->service.CookProject(request);
	AssetCookerBuildPublicResult(context, outResult);
	return context->lastServiceResult.exitCode;
}

int AssetCookerRecookAssets(
    AssetCookerContext* context,
    const AssetRecookRequest* request,
    AssetCookResult* outResult)
{
	if (context == nullptr)
	{
		return 1;
	}

	context->lastServiceResult = context->service.RecookAssets(request);
	AssetCookerBuildPublicResult(context, outResult);
	return context->lastServiceResult.exitCode;
}

int AssetCookerQueryCapabilities(AssetCookerContext* context, AssetCookerCapabilities* outCapabilities)
{
	if (context == nullptr || outCapabilities == nullptr)
	{
		return 1;
	}

	*outCapabilities = context->service.QueryCapabilities();
	return 0;
}

std::uint32_t AssetCookerGetLastDiagnostics(
    AssetCookerContext* context,
    const AssetCookDiagnostic** outDiagnostics)
{
	if (outDiagnostics != nullptr)
	{
		*outDiagnostics = nullptr;
	}

	if (context == nullptr)
	{
		return 0;
	}

	if (outDiagnostics != nullptr && !context->lastDiagnosticViews.empty())
	{
		*outDiagnostics = context->lastDiagnosticViews.data();
	}

	return static_cast<std::uint32_t>(context->lastDiagnosticViews.size());
}
