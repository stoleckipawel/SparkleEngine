#pragma once

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifact.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct ReferencePathTracerEncodedArtifact final
{
	std::uint32_t Width = 0u;
	std::uint32_t Height = 0u;
	std::vector<std::byte> Beauty;
	std::optional<std::vector<std::byte>> Checkpoint;
};

class ReferencePathTracerArtifactEncoding final
{
public:
	static bool Encode(
	    const ReferencePathTracerArtifactWriteRequest& request,
	    ReferencePathTracerEncodedArtifact& artifact,
	    std::string& errorMessage);
	static std::string BuildManifest(
	    const ReferencePathTracerArtifactWriteRequest& request,
	    const ReferencePathTracerEncodedArtifact& artifact,
	    std::string_view beautyHash,
	    std::string_view checkpointHash);
};
