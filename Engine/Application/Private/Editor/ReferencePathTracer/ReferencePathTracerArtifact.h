#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

enum class ReferencePathTracerArtifactKind : std::uint8_t
{
	PartialPrefix = 0,
	Complete = 1,
	Checkpoint = 2,
};

struct ReferencePathTracerArtifactWriteRequest final
{
	ReferencePathTracerArtifactKind Kind = ReferencePathTracerArtifactKind::PartialPrefix;
	std::filesystem::path AllowedRoot;
	std::filesystem::path PublicationDirectory;
	std::uint64_t MaximumOutputBytes = 64ull * 1024ull * 1024ull * 1024ull;
	ViewportCaptureReadback Mean;
	std::optional<ViewportCaptureReadback> Moment2;
};

struct ReferencePathTracerArtifactWriteResult final
{
	bool Succeeded = false;
	std::filesystem::path PublicationDirectory;
	std::string ManifestSha256;
	std::string ErrorMessage;
};
