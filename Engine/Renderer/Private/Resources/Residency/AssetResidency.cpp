#include "../../PCH.h"

#include "Resources/Residency/AssetResidency.h"

#include "RHI/Public/Commands/RhiCommandSubmissionService.h"

#include <algorithm>
#include <array>
#include <limits>

AssetResidency::AssetResidency(AssetResidencyBudget budget) noexcept :
    m_budget(budget)
{
}

std::optional<AssetGenerationHandle> AssetResidency::BeginGeneration(
    std::uint64_t assetKey,
    std::uint32_t generation) noexcept
{
	if (assetKey == 0 || generation == 0 ||
	    m_counters.RequestBacklog >= m_budget.MaximumRequestBacklog)
	{
		return std::nullopt;
	}

	const AssetGenerationHandle handle{.AssetKey = assetKey, .Generation = generation};
	const auto existing = std::find_if(
	    m_generations.begin(),
	    m_generations.end(),
	    [handle](const AssetGenerationStatus& candidate) noexcept
	    {
		    return candidate.Handle == handle;
	    });
	if (existing != m_generations.end() &&
	    existing->State != AssetResidencyState::Retired)
	{
		return handle;
	}
	if (existing != m_generations.end())
	{
		m_generations.erase(existing);
	}

	const auto newerGeneration = std::find_if(
	    m_generations.begin(),
	    m_generations.end(),
	    [handle](const AssetGenerationStatus& candidate) noexcept
	    {
		    return candidate.Handle.AssetKey == handle.AssetKey &&
		           candidate.Handle.Generation > handle.Generation;
	    });
	if (newerGeneration != m_generations.end())
	{
		return std::nullopt;
	}

	PruneTerminalGenerations();
	m_generations.push_back(
	    AssetGenerationStatus{
	        .Handle = handle,
	        .State = AssetResidencyState::Reading,
	        .BacklogAccounted = true});
	++m_counters.RequestBacklog;
	return handle;
}

bool AssetResidency::BeginDecoding(AssetGenerationHandle handle) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr || generation->State != AssetResidencyState::Reading)
	{
		return false;
	}
	generation->State = AssetResidencyState::Decoding;
	return true;
}

bool AssetResidency::PublishReadyForUpload(
    AssetGenerationHandle handle,
    std::uint64_t decodedBytes,
    std::uint64_t uploadBytes) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr ||
	    (generation->State != AssetResidencyState::Reading &&
	     generation->State != AssetResidencyState::Decoding) ||
	    !CanPublishDecoded(decodedBytes, uploadBytes))
	{
		return false;
	}

	generation->State = AssetResidencyState::ReadyForUpload;
	generation->DecodedBytes = decodedBytes;
	generation->UploadBytes = uploadBytes;
	m_counters.DecodedBytes += decodedBytes;
	return true;
}

bool AssetResidency::BeginUpload(AssetGenerationHandle handle) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr ||
	    generation->State != AssetResidencyState::ReadyForUpload ||
	    generation->UploadBytes > m_budget.MaximumPendingUploadBytes - m_counters.PendingUploadBytes ||
	    generation->UploadBytes > m_budget.MaximumResidentBytes - m_counters.ResidentBytes)
	{
		return false;
	}

	generation->State = AssetResidencyState::Uploading;
	m_counters.PendingUploadBytes += generation->UploadBytes;
	generation->PendingUploadAccounted = true;
	return true;
}

bool AssetResidency::RecordUploadSubmission(
    AssetGenerationHandle handle,
    RhiSubmissionToken completionToken,
    std::uint64_t residentBytes) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr ||
	    (generation->State != AssetResidencyState::Uploading &&
	     generation->State != AssetResidencyState::Evicting) ||
	    !completionToken.IsValid() ||
	    residentBytes > m_budget.MaximumResidentBytes - m_counters.ResidentBytes)
	{
		return false;
	}

	generation->Completion.MarkUsed(completionToken);
	generation->ResidentBytes = residentBytes;
	return true;
}

bool AssetResidency::Cancel(AssetGenerationHandle handle) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr)
	{
		return false;
	}

	if (generation->State == AssetResidencyState::Uploading)
	{
		generation->State = AssetResidencyState::Evicting;
		return true;
	}
	if (generation->State == AssetResidencyState::Resident)
	{
		return BeginEviction(handle, generation->Completion);
	}
	if (generation->State == AssetResidencyState::Evicting ||
	    generation->State == AssetResidencyState::Retired)
	{
		return false;
	}

	Retire(*generation);
	return true;
}

bool AssetResidency::BeginEviction(
    AssetGenerationHandle handle,
    const RhiSubmissionState& lastUse) noexcept
{
	AssetGenerationStatus* generation = FindMutable(handle);
	if (generation == nullptr ||
	    (generation->State != AssetResidencyState::Resident &&
	     generation->State != AssetResidencyState::Uploading))
	{
		return false;
	}

	generation->Completion = lastUse;
	generation->State = AssetResidencyState::Evicting;
	return true;
}

void AssetResidency::Poll(RhiCommandSubmissionService& submissions) noexcept
{
	for (AssetGenerationStatus& generation : m_generations)
	{
		if (generation.State == AssetResidencyState::Uploading &&
		    IsComplete(generation.Completion, submissions))
		{
			if (generation.PendingUploadAccounted &&
			    generation.UploadBytes <= m_counters.PendingUploadBytes)
			{
				m_counters.PendingUploadBytes -= generation.UploadBytes;
				generation.PendingUploadAccounted = false;
			}
			ReleaseCpuBudget(generation);
			generation.State = AssetResidencyState::Resident;
			m_counters.ResidentBytes += generation.ResidentBytes;
			generation.ResidentAccounted = true;
			ReleaseBacklog(generation);
		}
		else if (generation.State == AssetResidencyState::Evicting &&
		         IsComplete(generation.Completion, submissions))
		{
			Retire(generation);
		}
	}
}

const AssetGenerationStatus* AssetResidency::Find(AssetGenerationHandle handle) const noexcept
{
	const auto generation = std::find_if(
	    m_generations.begin(),
	    m_generations.end(),
	    [handle](const AssetGenerationStatus& candidate) noexcept
	    {
		    return candidate.Handle == handle;
	    });
	return generation != m_generations.end() ? &*generation : nullptr;
}

AssetResidencyState AssetResidency::GetState(AssetGenerationHandle handle) const noexcept
{
	const AssetGenerationStatus* generation = Find(handle);
	return generation != nullptr ? generation->State : AssetResidencyState::Unloaded;
}

AssetGenerationStatus* AssetResidency::FindMutable(AssetGenerationHandle handle) noexcept
{
	return const_cast<AssetGenerationStatus*>(std::as_const(*this).Find(handle));
}

bool AssetResidency::CanPublishDecoded(
    std::uint64_t decodedBytes,
    std::uint64_t uploadBytes) const noexcept
{
	return decodedBytes <= m_budget.MaximumDecodedBytes - m_counters.DecodedBytes &&
	       uploadBytes <= m_budget.MaximumPendingUploadBytes;
}

bool AssetResidency::IsComplete(
    const RhiSubmissionState& completion,
    RhiCommandSubmissionService& submissions) const noexcept
{
	std::array<RhiSubmissionToken, RhiQueueTypeCount> tokens{};
	const std::size_t tokenCount = completion.CopyTokens(tokens);
	for (std::size_t tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex)
	{
		if (!submissions.IsSubmissionComplete(tokens[tokenIndex]))
		{
			return false;
		}
	}
	return true;
}

void AssetResidency::ReleaseCpuBudget(AssetGenerationStatus& generation) noexcept
{
	if (generation.DecodedBytes <= m_counters.DecodedBytes)
	{
		m_counters.DecodedBytes -= generation.DecodedBytes;
	}
	generation.DecodedBytes = 0;
}

void AssetResidency::ReleaseBacklog(AssetGenerationStatus& generation) noexcept
{
	if (generation.BacklogAccounted && m_counters.RequestBacklog != 0)
	{
		--m_counters.RequestBacklog;
		generation.BacklogAccounted = false;
	}
}

void AssetResidency::Retire(AssetGenerationStatus& generation) noexcept
{
	if (generation.PendingUploadAccounted &&
	    generation.UploadBytes <= m_counters.PendingUploadBytes)
	{
		m_counters.PendingUploadBytes -= generation.UploadBytes;
		generation.PendingUploadAccounted = false;
	}
	if (generation.ResidentAccounted &&
	    generation.ResidentBytes <= m_counters.ResidentBytes)
	{
		m_counters.ResidentBytes -= generation.ResidentBytes;
		generation.ResidentAccounted = false;
	}
	ReleaseCpuBudget(generation);
	generation.State = AssetResidencyState::Retired;
	ReleaseBacklog(generation);
}

void AssetResidency::PruneTerminalGenerations() noexcept
{
	if (m_generations.size() < m_budget.MaximumRequestBacklog)
	{
		return;
	}

	m_generations.erase(
	    std::remove_if(
	        m_generations.begin(),
	        m_generations.end(),
	        [](const AssetGenerationStatus& generation) noexcept
	        {
		        return generation.State == AssetResidencyState::Retired;
	        }),
	    m_generations.end());
}
