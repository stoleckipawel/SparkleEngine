// ============================================================================
// PathRoot.h
// Identifies the root scope for resolving engine paths (Project vs Engine).
// ----------------------------------------------------------------------------
// USAGE:
//   auto path = assetSystem.GetShaderPath(PathRoot::Engine);
//   auto name = GetPathRootName(PathRoot::Project);  // "Project"
//
// DESIGN:
//   - Project paths can override engine defaults
//   - PathRoot::Any checks Project first, then Engine
// ============================================================================
#pragma once

#include <cstdint>
#include <string_view>

enum class PathRoot : uint8_t
{
	Any,      // Check Project first, then Engine (default)
	Project,  // Game paths only
	Engine,   // Built-in engine paths only

	Count
};

constexpr std::string_view GetPathRootName(PathRoot root) noexcept
{
	switch (root)
	{
		case PathRoot::Any:
			return "Any";
		case PathRoot::Project:
			return "Project";
		case PathRoot::Engine:
			return "Engine";
		case PathRoot::Count:
		default:
			return "Unknown";
	}
}
