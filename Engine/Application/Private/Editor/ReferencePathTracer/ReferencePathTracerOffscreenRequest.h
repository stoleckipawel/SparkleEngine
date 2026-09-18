#pragma once

#include "RHI/Public/Core/RhiBackendSelection.h"

#include <cstdint>
#include <filesystem>
#include <string>

struct ReferencePathTracerOffscreenRequest final
{
	std::filesystem::path ProjectRoot;
	std::string LevelId;
	std::filesystem::path OutputDirectory;
	ERhiBackendApi Backend = ERhiBackendApi::Unknown;
	std::uint32_t Width = 0u;
	std::uint32_t Height = 0u;
	std::uint32_t TimeoutSeconds = 28800u;
	std::uint64_t MaximumOutputBytes = 64ull * 1024ull * 1024ull * 1024ull;
	bool CheckpointOnTimeout = false;
};

bool ReadReferencePathTracerOffscreenRequest(
    const std::filesystem::path& path,
    ReferencePathTracerOffscreenRequest& request,
    std::string& errorMessage);
