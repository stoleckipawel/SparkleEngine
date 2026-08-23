#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderRecookCoordinator.h"
#include "Core/Public/Diagnostics/Error.h"
#include "EditorOperations/EditorOperationService.h"

#include "Renderer.h"
#include "ShaderRecook/ShaderRecookPublicationReader.h"

#include <algorithm>
#include <format>
#include <utility>

ShaderRecookCoordinator::ShaderRecookCoordinator(EditorOperationService& operations) :
    m_operations(&operations)
{
}

ShaderRecookCoordinator::~ShaderRecookCoordinator() = default;

void ShaderRecookCoordinator::SetStatusHandler(StatusHandler handler)
{
	m_statusHandler = std::move(handler);
}

void ShaderRecookCoordinator::RequestRecook() noexcept
{
	RequestRecook(ShaderRecookRequest{.Type = ShaderRecookRequestType::Global});
}

void ShaderRecookCoordinator::RequestRecook(ShaderRecookRequest request) noexcept
{
	if (request.Type == ShaderRecookRequestType::Changed && request.ChangedVirtualPaths.empty())
	{
		request.ChangedVirtualPaths = m_shaderSourceChangeTracker.CollectChangedVirtualPaths();
		if (request.ChangedVirtualPaths.empty())
		{
			PublishStatus("No changed shader source paths are pending; no shader compiler process was launched.");
			return;
		}
	}

	if (m_hasActiveRecook)
	{
		if (m_hasQueuedRecook && m_queuedRequest.Type == ShaderRecookRequestType::Changed
		    && request.Type == ShaderRecookRequestType::Changed)
		{
			m_queuedRequest.ChangedVirtualPaths.insert(
			    m_queuedRequest.ChangedVirtualPaths.end(),
			    request.ChangedVirtualPaths.begin(),
			    request.ChangedVirtualPaths.end());
			std::ranges::sort(m_queuedRequest.ChangedVirtualPaths);
			m_queuedRequest.ChangedVirtualPaths.erase(
			    std::unique(m_queuedRequest.ChangedVirtualPaths.begin(), m_queuedRequest.ChangedVirtualPaths.end()),
			    m_queuedRequest.ChangedVirtualPaths.end());
		}
		else if (!m_hasQueuedRecook || m_queuedRequest.Type != ShaderRecookRequestType::Global)
		{
			m_queuedRequest = std::move(request);
			m_hasQueuedRecook = true;
		}
		PublishStatus(std::format("Shader recook already running; queued one follow-up request for {}.", DescribeRequest(m_queuedRequest)));
		return;
	}

	StartRecook(std::move(request));
}

void ShaderRecookCoordinator::RequestReload() noexcept
{
	m_reloadRequested = true;
	PublishStatus("Shader reload requested; the cooked shader map and code library will reload at the next coordinator update.");
}

void ShaderRecookCoordinator::Update(Renderer& renderer, bool reloadRequested) noexcept
{
	std::vector<std::string> changedVirtualPaths = m_shaderSourceChangeTracker.CollectChangedVirtualPaths();
	if (!changedVirtualPaths.empty())
	{
		PublishStatus(
		    std::format(
		        "Detected {} changed shader source path(s); scheduling dependency-directed compilation.",
		        changedVirtualPaths.size()));
		RequestRecook(ShaderRecookRequest{.Type = ShaderRecookRequestType::Changed, .ChangedVirtualPaths = std::move(changedVirtualPaths)});
	}

	if (m_reloadRequested || reloadRequested)
	{
		m_reloadRequested = false;
		HandleManualReload(renderer);
	}

	if (!m_hasActiveRecook)
	{
		HandleExternalRecookPublication(renderer);
		return;
	}

	ShaderRecookExecutionResult result;
	if (!m_operations->TryConsumeShaderRecook(result))
	{
		return;
	}

	m_hasActiveRecook = false;
	CompleteRecook(renderer, std::move(result));

	if (m_hasQueuedRecook)
	{
		ShaderRecookRequest queuedRequest = std::move(m_queuedRequest);
		m_queuedRequest = {};
		m_hasQueuedRecook = false;
		StartRecook(std::move(queuedRequest));
	}

	if (!m_hasActiveRecook)
	{
		HandleExternalRecookPublication(renderer);
	}
}

void ShaderRecookCoordinator::StartRecook(ShaderRecookRequest request) noexcept
{
	const std::uint64_t requestId = m_nextRequestId++;
	const std::uint64_t baselinePublicationId = ReadCurrentPublicationId();
	std::string errorMessage;
	if (!m_operations->StartShaderRecook(requestId, baselinePublicationId, request, errorMessage))
	{
		PublishStatus("Shader recook failed before launch: " + errorMessage);
		return;
	}
	m_latestRequestId = requestId;
	m_hasActiveRecook = true;
	PublishStatus(
	    std::format(
	        "Shader recook #{} started for {} through the shader compiler process (baselinePublicationId={}).",
	        requestId,
	        DescribeRequest(request),
	        baselinePublicationId));
}

void ShaderRecookCoordinator::CompleteRecook(Renderer& renderer, ShaderRecookExecutionResult result) noexcept
{
	if (result.RequestId != m_latestRequestId)
	{
		return;
	}
	if (result.Process.NoWork())
	{
		PublishStatus(
		    std::format(
		        "Shader recook #{} ({}) found no affected registered shader types; no compilation or publication was performed.",
		        result.RequestId,
		        DescribeRequest(result.Request)));
		return;
	}

	if (result.Process.Succeeded())
	{
		const ShaderRecookPublicationReadResult readResult = ReadRecookPublication();
		ShaderRecookPublication publication;
		std::string publicationDiagnostic;
		if (!TryAcceptFreshPublication(readResult, result.BaselinePublicationId, publication, publicationDiagnostic))
		{
			PublishStatus(
			    std::format(
			        "Shader recook #{} ({}) finished with process success, but runtime reload was rejected before touching active "
			        "shader artifacts: {}\nCommand: {}\n\n{}",
			        result.RequestId,
			        DescribeRequest(result.Request),
			        publicationDiagnostic,
			        result.Process.CommandLine,
			        result.Process.Output));
			return;
		}

		try
		{
			ReloadShaders(renderer);
		}
		catch (const Diagnostics::Error& error)
		{
			m_lastAcceptedPublicationId = publication.PublicationId;
			m_hasAcceptedPublication = true;
			PublishStatus(
			    std::format(
			        "Shader recook #{} ({}) published fresh result {} but runtime validation rejected the replacement set; previous cooked "
			        "shader map remains active. {}\nCommand: {}\n\n{}",
			        result.RequestId,
			        DescribeRequest(result.Request),
			        publication.PublicationId,
			        error.what(),
			        result.Process.CommandLine,
			        result.Process.Output));
			return;
		}

		m_lastAcceptedPublicationId = publication.PublicationId;
		m_hasAcceptedPublication = true;
		PublishStatus(
		    std::format(
		        "Shader recook #{} ({}) succeeded. Accepted publication {} and activated a validated shader runtime generation.\nCommand: "
		        "{}\n\n{}",
		        result.RequestId,
		        DescribeRequest(result.Request),
		        publication.PublicationId,
		        result.Process.CommandLine,
		        result.Process.Output));
		return;
	}

	PublishStatus(
	    std::format(
	        "Shader recook #{} ({}) failed with exit code {}. The previous shader map remains active; no recook publication was "
	        "accepted and old artifacts remain loaded.\nCommand: {}\n\n{}",
	        result.RequestId,
	        DescribeRequest(result.Request),
	        result.Process.ExitCode,
	        result.Process.CommandLine,
	        result.Process.Output));
}

void ShaderRecookCoordinator::ReloadShaders(Renderer& renderer)
{
	PublishStatus("Shader reload accepted; validating a replacement runtime generation at the render boundary.");
	renderer.ReloadShaders();
}

void ShaderRecookCoordinator::HandleManualReload(Renderer& renderer) noexcept
{
	try
	{
		ReloadShaders(renderer);
		PublishStatus(
		    std::format(
		        "Manual shader reload activated generation {} without a device-idle drain.",
		        renderer.GetShaderGeneration()));
	}
	catch (const Diagnostics::Error& error)
	{
		PublishStatus(
		    std::format(
		        "Manual shader reload was rejected by runtime validation; the previous shader map remains active. {}",
		        error.what()));
	}
}

void ShaderRecookCoordinator::HandleExternalRecookPublication(Renderer& renderer) noexcept
{
	const ShaderRecookPublicationReadResult readResult = ReadRecookPublication();
	if (readResult.Missing)
	{
		return;
	}

	if (!readResult.Diagnostic.empty())
	{
		if (readResult.Diagnostic != m_lastPublicationDiagnostic)
		{
			m_lastPublicationDiagnostic = readResult.Diagnostic;
			PublishStatus(readResult.Diagnostic);
		}
		return;
	}

	if (!readResult.Publication.has_value())
	{
		return;
	}

	if (!m_hasAcceptedPublication)
	{
		m_lastAcceptedPublicationId = readResult.Publication->PublicationId;
		m_hasAcceptedPublication = true;
		m_lastPublicationDiagnostic.clear();
		return;
	}

	if (readResult.Publication->PublicationId <= m_lastAcceptedPublicationId)
	{
		m_lastPublicationDiagnostic.clear();
		return;
	}

	ShaderRecookPublication publication;
	std::string publicationDiagnostic;
	if (!TryAcceptFreshPublication(readResult, m_lastAcceptedPublicationId, publication, publicationDiagnostic))
	{
		if (!publicationDiagnostic.empty() && publicationDiagnostic != m_lastPublicationDiagnostic)
		{
			m_lastPublicationDiagnostic = publicationDiagnostic;
			PublishStatus(publicationDiagnostic);
		}
		return;
	}

	try
	{
		ReloadShaders(renderer);
	}
	catch (const Diagnostics::Error& error)
	{
		m_lastAcceptedPublicationId = publication.PublicationId;
		m_hasAcceptedPublication = true;
		m_lastPublicationDiagnostic.clear();
		PublishStatus(
		    std::format(
		        "External shader recook publication {} was fresh, but runtime validation rejected the replacement set; previous cooked "
		        "shader map remains active. {}",
		        publication.PublicationId,
		        error.what()));
		return;
	}

	m_lastAcceptedPublicationId = publication.PublicationId;
	m_hasAcceptedPublication = true;
	m_lastPublicationDiagnostic.clear();
	PublishStatus(
	    std::format(
	        "External shader recook publication {} accepted and activated without a device-idle drain (generation={}).",
	        publication.PublicationId,
	        renderer.GetShaderGeneration()));
}

void ShaderRecookCoordinator::PublishStatus(std::string status) noexcept
{
	if (!m_statusHandler)
	{
		return;
	}

	try
	{
		m_statusHandler(std::move(status));
	}
	catch (...)
	{
		return;
	}
}

std::string ShaderRecookCoordinator::DescribeRequest(const ShaderRecookRequest& request)
{
	switch (request.Type)
	{
		case ShaderRecookRequestType::Changed:
			return std::format("{} changed shader source path(s)", request.ChangedVirtualPaths.size());
		case ShaderRecookRequestType::ShaderId:
			return request.Target.empty() ? "shader id <empty>" : "shader id '" + request.Target + "'";
		case ShaderRecookRequestType::Global:
		default:
			return "all global shaders";
	}
}

std::uint64_t ShaderRecookCoordinator::ReadCurrentPublicationId() noexcept
{
	const ShaderRecookPublicationReadResult readResult = ReadRecookPublication();
	if (readResult.Publication.has_value())
	{
		m_lastAcceptedPublicationId = readResult.Publication->PublicationId;
		m_hasAcceptedPublication = true;
		return readResult.Publication->PublicationId;
	}

	return m_hasAcceptedPublication ? m_lastAcceptedPublicationId : 0;
}

ShaderRecookPublicationReadResult ShaderRecookCoordinator::ReadRecookPublication() noexcept
{
	return ShaderRecookPublicationReader::Read(Filesystem::GetShaderRecookSignalPath());
}

bool ShaderRecookCoordinator::TryAcceptFreshPublication(
    const ShaderRecookPublicationReadResult& readResult,
    std::uint64_t minimumPublicationId,
    ShaderRecookPublication& outPublication,
    std::string& outDiagnostic) noexcept
{
	if (readResult.Missing)
	{
		outDiagnostic =
		    "Shader compiler process succeeded, but no recook publication file was found; reload rejected before touching active shaders.";
		return false;
	}

	if (!readResult.Diagnostic.empty())
	{
		outDiagnostic = readResult.Diagnostic;
		return false;
	}

	if (!readResult.Publication.has_value())
	{
		outDiagnostic = "Shader recook publication was not readable; reload rejected before touching active shaders.";
		return false;
	}

	const ShaderRecookPublication& publication = *readResult.Publication;
	const std::uint64_t freshnessFloor = std::max(minimumPublicationId, m_hasAcceptedPublication ? m_lastAcceptedPublicationId : 0ull);
	if (publication.PublicationId <= freshnessFloor)
	{
		outDiagnostic = std::format(
		    "Shader recook publication {} is stale; expected a publication newer than {}. Reload rejected before touching active shaders.",
		    publication.PublicationId,
		    freshnessFloor);
		return false;
	}

	outPublication = publication;
	outDiagnostic.clear();
	return true;
}
