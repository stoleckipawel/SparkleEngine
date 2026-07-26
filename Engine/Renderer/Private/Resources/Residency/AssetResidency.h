#pragma once

#include "RHI/Public/Commands/RhiQueue.h"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

class RhiCommandSubmissionService;

enum class AssetResidencyState : std::uint8_t
{
	Unloaded,
	Reading,
	Decoding,
	ReadyForUpload,
	Uploading,
	Resident,
	Evicting,
	Retired,
	Failed,
};

struct AssetGenerationHandle final
{
	std::uint64_t AssetKey = 0;
	std::uint32_t Generation = 0;

	constexpr bool IsValid() const noexcept { return AssetKey != 0 && Generation != 0; }
	constexpr auto operator<=>(const AssetGenerationHandle&) const noexcept = default;
};

struct AssetResidencyBudget final
{
	std::uint64_t MaximumDecodedBytes = 512ull * 1024ull * 1024ull;
	std::uint64_t MaximumPendingUploadBytes = 256ull * 1024ull * 1024ull;
	std::uint64_t MaximumResidentBytes = 2ull * 1024ull * 1024ull * 1024ull;
	std::uint32_t MaximumRequestBacklog = 256;
};

struct AssetResidencyCounters final
{
	std::uint64_t DecodedBytes = 0;
	std::uint64_t PendingUploadBytes = 0;
	std::uint64_t ResidentBytes = 0;
	std::uint32_t RequestBacklog = 0;
};

struct AssetGenerationStatus final
{
	AssetGenerationHandle Handle;
	AssetResidencyState State = AssetResidencyState::Unloaded;
	std::uint64_t DecodedBytes = 0;
	std::uint64_t UploadBytes = 0;
	std::uint64_t ResidentBytes = 0;
	RhiSubmissionState Completion;
	bool BacklogAccounted = false;
	bool PendingUploadAccounted = false;
	bool ResidentAccounted = false;
};

class AssetResidency final
{
  public:
	explicit AssetResidency(AssetResidencyBudget budget = {}) noexcept;

	std::optional<AssetGenerationHandle> BeginGeneration(
	    std::uint64_t assetKey,
	    std::uint32_t generation) noexcept;
	bool BeginDecoding(AssetGenerationHandle handle) noexcept;
	bool PublishReadyForUpload(
	    AssetGenerationHandle handle,
	    std::uint64_t decodedBytes,
	    std::uint64_t uploadBytes) noexcept;
	bool BeginUpload(AssetGenerationHandle handle) noexcept;
	bool RecordUploadSubmission(
	    AssetGenerationHandle handle,
	    RhiSubmissionToken completionToken,
	    std::uint64_t residentBytes) noexcept;
	bool MarkFailed(AssetGenerationHandle handle) noexcept;
	bool Cancel(AssetGenerationHandle handle) noexcept;
	bool BeginEviction(
	    AssetGenerationHandle handle,
	    const RhiSubmissionState& lastUse) noexcept;
	void Poll(RhiCommandSubmissionService& submissions) noexcept;

	const AssetGenerationStatus* Find(AssetGenerationHandle handle) const noexcept;
	AssetResidencyState GetState(AssetGenerationHandle handle) const noexcept;
	const AssetResidencyBudget& GetBudget() const noexcept { return m_budget; }
	const AssetResidencyCounters& GetCounters() const noexcept { return m_counters; }

  private:
	AssetGenerationStatus* FindMutable(AssetGenerationHandle handle) noexcept;
	bool CanPublishDecoded(
	    std::uint64_t decodedBytes,
	    std::uint64_t uploadBytes) const noexcept;
	bool IsComplete(
	    const RhiSubmissionState& completion,
	    RhiCommandSubmissionService& submissions) const noexcept;
	void ReleaseCpuBudget(AssetGenerationStatus& generation) noexcept;
	void ReleaseBacklog(AssetGenerationStatus& generation) noexcept;
	void Retire(AssetGenerationStatus& generation) noexcept;
	void PruneTerminalGenerations() noexcept;

	AssetResidencyBudget m_budget;
	AssetResidencyCounters m_counters;
	std::vector<AssetGenerationStatus> m_generations;
};
