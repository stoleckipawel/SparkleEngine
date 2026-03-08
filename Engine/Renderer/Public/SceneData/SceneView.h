// =============================================================================
// SceneView.h — Per-Frame Scene Representation for Rendering
// =============================================================================
//
// Pure-data structure representing everything the renderer needs to draw a
// single frame. Built by Renderer::BuildSceneView() each frame.
//
// DESIGN:
//   - Frame-scoped data assembled by Renderer from longer-lived engine state
//   - Camera stored as pointer to renderer-owned RenderCamera
//   - Material payloads may reference renderer-owned GPU resources such as
//     persistent descriptor-table handles, but SceneView does not own them
//
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <cstdint>
#include <vector>

class RenderCamera;

// =============================================================================
// SceneView
// =============================================================================

struct SPARKLE_RENDERER_API SceneView
{
	// -------------------------------------------------------------------------
	// Camera
	// -------------------------------------------------------------------------

	const RenderCamera* camera = nullptr;

	// -------------------------------------------------------------------------
	// Viewport
	// -------------------------------------------------------------------------

	std::uint32_t width = 0;
	std::uint32_t height = 0;

	// -------------------------------------------------------------------------
	// Lighting
	// -------------------------------------------------------------------------

	DirectionalLight sunLight = {};

	// -------------------------------------------------------------------------
	// Draw Commands
	// -------------------------------------------------------------------------

	std::vector<MeshDraw> meshDraws;
	std::vector<MaterialData> materials;
};
