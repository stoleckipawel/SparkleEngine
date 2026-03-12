// ============================================================================
// Level.h
// ----------------------------------------------------------------------------
// Abstract base class for levels. A Level defines the initial content
// that populates a Scene: geometry, lighting setup, etc.
//
#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <string_view>

// ============================================================================
// Level
// ============================================================================

class SPARKLE_ENGINE_API Level
{
  public:
	virtual ~Level() = default;

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;
	Level(Level&&) = delete;
	Level& operator=(Level&&) = delete;

	// ========================================================================
	// Identity
	// ========================================================================

	/// Unique level name used for registry lookup (e.g., "Sponza").
	virtual std::string_view GetName() const = 0;

	/// Human-readable description (e.g., "Sponza Palace - PBR test scene").
	virtual std::string_view GetDescription() const = 0;

	// ========================================================================
	// Description
	// ========================================================================

	/// Returns a declarative description of the level's content.
	virtual LevelDesc BuildDescription() const = 0;

  protected:
	Level() = default;
};
