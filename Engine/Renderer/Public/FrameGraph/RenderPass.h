// ============================================================================
// RenderPass.h
// ----------------------------------------------------------------------------
// Abstract base class for Frame Graph render passes.
//
// PURPOSE:
//   Defines the interface for all render passes in the Frame Graph system.
//   Passes declare resource dependencies in Setup, then record GPU commands
//   in Execute.
//
#pragma once

#include <string>
#include <string_view>

// Forward declarations
class PassBuilder;
class RenderContext;
struct SceneView;

// ============================================================================
// RenderPass
// ============================================================================

/// Abstract base class for Frame Graph render passes.
/// Implements the two-phase Setup/Execute pattern from Frostbite's Frame Graph.
class RenderPass
{
  public:
	// -------------------------------------------------------------------------
	// Construction / Destruction
	// -------------------------------------------------------------------------

	/// Constructs a render pass with the given debug name.
	/// @param name Pass name for debugging, profiling, and GPU markers
	explicit RenderPass(std::string_view name) noexcept : m_name(name) {}

	/// Virtual destructor for proper cleanup of derived classes.
	virtual ~RenderPass() noexcept = default;

	// Non-copyable, non-movable (passes are managed by FrameGraph)
	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
	RenderPass(RenderPass&&) = delete;
	RenderPass& operator=(RenderPass&&) = delete;

	// -------------------------------------------------------------------------
	// Pass Interface
	// -------------------------------------------------------------------------

	/// Declares resource dependencies for this pass.
	/// Called once per frame before Execute. NO GPU WORK HERE.
	///
	/// @param builder  PassBuilder for declaring resource reads/writes
	/// @param sceneView Scene data (camera, meshes, materials, lights)
	virtual void Setup(PassBuilder& builder, const SceneView& sceneView) = 0;

	/// Records GPU commands for this pass.
	/// Called once per frame after all Setup calls complete.
	///
	/// @param context RenderContext for issuing draw commands
	virtual void Execute(RenderContext& context) = 0;

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------

	/// Returns the pass name for debugging/profiling.
	const std::string& GetName() const noexcept { return m_name; }

  protected:
	std::string m_name;
};
