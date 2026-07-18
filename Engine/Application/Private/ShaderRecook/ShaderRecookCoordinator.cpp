#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderRecookCoordinator.h"

#include "Renderer.h"
#include "ShaderRecook/ShaderRecookPublicationReader.h"
#include "Core/Public/Threading/ThreadOwnership.h"

#include <algorithm>
#include <exception>
#include <format>
#include <utility>

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
	if (m_hasActiveRecook)
	{
		m_queuedRequest = std::move(request);
		m_hasQueuedRecook = true;
		PublishStatus(std::format("Shader recook already running; queued one follow-up request for {}.", DescribeRequest(m_queuedRequest)));
		return;
	}

	StartRecook(std::move(request));
}

void ShaderRecookCoordinator::RequestReload() noexcept
{
	m_reloadRequested = true;
	PublishStatus("Shader reload requested; cooked shader packages will reload at the next coordinator update.");
}

void ShaderRecookCoordinator::Update(Renderer& renderer, bool reloadRequested) noexcept
{
	if (m_shaderSourceChangeTracker.HasChanged())
	{
		PublishStatus("Shader source change detected; scheduling out-of-process recook.");
		RequestRecook(ShaderRecookRequest{.Type = ShaderRecookRequestType::Changed});
	}

	if (m_reloadRequested || reloadRequested)
	{
		m_reloadRequested = false;
		HandleManualReload(renderer);
	}

	if (!m_hasActiveRecook || !m_recookFuture.valid())
	{
		HandleExternalRecookPublication(renderer);
		return;
	}

	using namespace std::chrono_literals;
	if (m_recookFuture.wait_for(0ms) != std::future_status::ready)
	{
		return;
	}

	ProcessResult result;
	try
	{
		result = m_recookFuture.get();
	}
	catch (const std::exception& exception)
	{
		result.RequestId = m_activeRequestId;
		result.BaselinePublicationId = m_activeBaselinePublicationId;
		result.Request = m_activeRequest;
		result.Process.Output = exception.what();
	}
	catch (...)
	{
		result.RequestId = m_activeRequestId;
		result.BaselinePublicationId = m_activeBaselinePublicationId;
		result.Request = m_activeRequest;
		result.Process.Output = "Unknown shader recook worker failure.";
	}

	m_hasActiveRecook = false;
	m_activeRequestId = 0;
	m_activeBaselinePublicationId = 0;
	m_activeRequest = {};
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
	try
	{
		const std::uint64_t requestId = m_nextRequestId++;
		const std::uint64_t baselinePublicationId = ReadCurrentPublicationId();
		m_activeRequestId = requestId;
		m_latestRequestId = requestId;
		m_activeBaselinePublicationId = baselinePublicationId;
		m_activeRequest = request;
		m_hasActiveRecook = true;
		m_recookFuture =
		    std::async(std::launch::async, &ShaderRecookCoordinator::RunRecookProcess, requestId, baselinePublicationId, request);

		PublishStatus(
		    std::format(
		        "Shader recook #{} started for {} through the shader compiler process (baselinePublicationId={}).",
		        requestId,
		        DescribeRequest(request),
		        baselinePublicationId));
	}
	catch (const std::exception& exception)
	{
		m_hasActiveRecook = false;
		m_activeRequestId = 0;
		m_activeBaselinePublicationId = 0;
		PublishStatus(std::string("Shader recook failed before launch: ") + exception.what());
	}
	catch (...)
	{
		m_hasActiveRecook = false;
		m_activeRequestId = 0;
		m_activeBaselinePublicationId = 0;
		PublishStatus("Shader recook failed before launch with an unknown error.");
	}
}

void ShaderRecookCoordinator::CompleteRecook(Renderer& renderer, ProcessResult result) noexcept
{
	if (result.RequestId != m_latestRequestId)
	{
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
			        "packages: {}\nCommand: {}\n\n{}",
			        result.RequestId,
			        DescribeRequest(result.Request),
			        publicationDiagnostic,
			        result.Process.CommandLine,
			        result.Process.Output));
			return;
		}

		const CookedShaderReloadResult reloadResult = ReloadCookedShaders(renderer);
		if (!reloadResult)
		{
			m_lastAcceptedPublicationId = publication.PublicationId;
			m_hasAcceptedPublication = true;
			PublishStatus(
			    std::format(
			        "Shader recook #{} ({}) published fresh result {} but runtime validation rejected the replacement set; previous cooked "
			        "shader packages remain active. {}\nCommand: {}\n\n{}",
			        result.RequestId,
			        DescribeRequest(result.Request),
			        publication.PublicationId,
			        reloadResult.ErrorMessage,
			        result.Process.CommandLine,
			        result.Process.Output));
			return;
		}

		m_lastAcceptedPublicationId = publication.PublicationId;
		m_hasAcceptedPublication = true;
		PublishStatus(
		    std::format(
		        "Shader recook #{} ({}) succeeded. Accepted publication {} and reloaded cooked shader packages after RHI idle.\nCommand: "
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
	        "Shader recook #{} ({}) failed with exit code {}. Previous cooked shader packages remain active; no recook publication was "
	        "accepted and old artifacts remain loaded.\nCommand: {}\n\n{}",
	        result.RequestId,
	        DescribeRequest(result.Request),
	        result.Process.ExitCode,
	        result.Process.CommandLine,
	        result.Process.Output));
}

CookedShaderReloadResult ShaderRecookCoordinator::ReloadCookedShaders(Renderer& renderer) noexcept
{
	PublishStatus("Shader reload accepted; waiting for RHI idle before replacing shader runtime state.");
	renderer.WaitForIdle();
	return renderer.ReloadCookedShaders();
}

void ShaderRecookCoordinator::HandleManualReload(Renderer& renderer) noexcept
{
	const CookedShaderReloadResult reloadResult = ReloadCookedShaders(renderer);
	if (reloadResult)
	{
		PublishStatus(std::format("Manual shader reload completed after RHI idle (generation={}).", renderer.GetShaderPackageGeneration()));
		return;
	}

	PublishStatus(
	    std::format(
	        "Manual shader reload was rejected by runtime validation; previous cooked shader packages remain active. {}",
	        reloadResult.ErrorMessage));
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

	const CookedShaderReloadResult reloadResult = ReloadCookedShaders(renderer);
	if (!reloadResult)
	{
		m_lastAcceptedPublicationId = publication.PublicationId;
		m_hasAcceptedPublication = true;
		m_lastPublicationDiagnostic.clear();
		PublishStatus(
		    std::format(
		        "External shader recook publication {} was fresh, but runtime validation rejected the replacement set; previous cooked "
		        "shader packages remain active. {}",
		        publication.PublicationId,
		        reloadResult.ErrorMessage));
		return;
	}

	m_lastAcceptedPublicationId = publication.PublicationId;
	m_hasAcceptedPublication = true;
	m_lastPublicationDiagnostic.clear();
	PublishStatus(
	    std::format(
	        "External shader recook publication {} accepted and reloaded after RHI idle (generation={}).",
	        publication.PublicationId,
	        renderer.GetShaderPackageGeneration()));
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
			return "changed shader sources";
		case ShaderRecookRequestType::PackageId:
			return request.Target.empty() ? "shader package <empty>" : "shader package '" + request.Target + "'";
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
		    "Shader compiler process succeeded, but no recook publication file was found; reload rejected before touching active packages.";
		return false;
	}

	if (!readResult.Diagnostic.empty())
	{
		outDiagnostic = readResult.Diagnostic;
		return false;
	}

	if (!readResult.Publication.has_value())
	{
		outDiagnostic = "Shader recook publication was not readable; reload rejected before touching active packages.";
		return false;
	}

	const ShaderRecookPublication& publication = *readResult.Publication;
	const std::uint64_t freshnessFloor = std::max(minimumPublicationId, m_hasAcceptedPublication ? m_lastAcceptedPublicationId : 0ull);
	if (publication.PublicationId <= freshnessFloor)
	{
		outDiagnostic = std::format(
		    "Shader recook publication {} is stale; expected a publication newer than {}. Reload rejected before touching active packages.",
		    publication.PublicationId,
		    freshnessFloor);
		return false;
	}

	outPublication = publication;
	outDiagnostic.clear();
	return true;
}

ShaderRecookCoordinator::ProcessResult ShaderRecookCoordinator::RunRecookProcess(
    std::uint64_t requestId,
    std::uint64_t baselinePublicationId,
    ShaderRecookRequest request) noexcept
{
	Threading::SetCurrentThreadRole("Sparkle.Tool.ShaderRecook");
	ProcessResult result;
	result.RequestId = requestId;
	result.BaselinePublicationId = baselinePublicationId;
	result.Request = request;
	result.Process = ShaderCompilerProcess::RunCook(request);
	return result;
}
