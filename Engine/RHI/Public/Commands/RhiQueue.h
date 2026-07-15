#pragma once

#include "../RHIAPI.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

enum class ERhiQueueType : std::uint8_t
{
	Graphics = 0,
	Compute,
	Copy,
	Count,
};

constexpr std::size_t RhiQueueTypeCount = static_cast<std::size_t>(ERhiQueueType::Count);

constexpr std::size_t RhiQueueTypeToIndex(ERhiQueueType queue) noexcept
{
	return static_cast<std::size_t>(queue);
}

constexpr const char* RhiQueueTypeToString(ERhiQueueType queue) noexcept
{
	switch (queue)
	{
		case ERhiQueueType::Graphics:
			return "Graphics";
		case ERhiQueueType::Compute:
			return "Compute";
		case ERhiQueueType::Copy:
			return "Copy";
		case ERhiQueueType::Count:
		default:
			return "Unknown";
	}
}

struct RhiSubmissionToken final
{
	ERhiQueueType Queue = ERhiQueueType::Graphics;
	std::uint64_t Value = 0;

	constexpr bool IsValid() const noexcept
	{
		return Queue != ERhiQueueType::Count && Value != 0;
	}

	constexpr explicit operator bool() const noexcept { return IsValid(); }

	constexpr bool operator==(const RhiSubmissionToken&) const noexcept = default;
};

struct RhiSubmissionState final
{
	std::array<std::uint64_t, RhiQueueTypeCount> Values{};

	constexpr void Clear() noexcept { Values.fill(0); }

	constexpr void MarkUsed(RhiSubmissionToken token) noexcept
	{
		if (token.IsValid())
		{
			std::uint64_t& value = Values[RhiQueueTypeToIndex(token.Queue)];
			value = value < token.Value ? token.Value : value;
		}
	}

	constexpr RhiSubmissionToken GetToken(ERhiQueueType queue) const noexcept
	{
		return queue != ERhiQueueType::Count
		           ? RhiSubmissionToken{.Queue = queue, .Value = Values[RhiQueueTypeToIndex(queue)]}
		           : RhiSubmissionToken{};
	}

	constexpr std::size_t CopyTokens(std::span<RhiSubmissionToken> destination) const noexcept
	{
		std::size_t count = 0;
		for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount && count < destination.size(); ++queueIndex)
		{
			const std::uint64_t value = Values[queueIndex];
			if (value != 0)
			{
				destination[count++] = RhiSubmissionToken{
				    .Queue = static_cast<ERhiQueueType>(queueIndex),
				    .Value = value};
			}
		}
		return count;
	}

	constexpr bool IsComplete(const std::array<std::uint64_t, RhiQueueTypeCount>& completedValues) const noexcept
	{
		for (std::size_t queueIndex = 0; queueIndex < RhiQueueTypeCount; ++queueIndex)
		{
			if (completedValues[queueIndex] < Values[queueIndex])
			{
				return false;
			}
		}
		return true;
	}
};
