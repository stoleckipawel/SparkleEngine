#pragma once

#include "FrameGraph/FrameGraphPassKind.h"

#include <cstdint>

enum class EFrameGraphQueuePreference : std::uint8_t
{
	Graphics,
	AsyncCompute,
	Copy,
};

constexpr bool IsQueuePreferenceCompatible(EFrameGraphPassKind passKind, EFrameGraphQueuePreference queuePreference) noexcept
{
	switch (queuePreference)
	{
		case EFrameGraphQueuePreference::Graphics:
			return IsValidFrameGraphPassKind(passKind);
		case EFrameGraphQueuePreference::AsyncCompute:
			return passKind == EFrameGraphPassKind::Compute;
		case EFrameGraphQueuePreference::Copy:
			return passKind == EFrameGraphPassKind::Transfer;
		default:
			return false;
	}
}
