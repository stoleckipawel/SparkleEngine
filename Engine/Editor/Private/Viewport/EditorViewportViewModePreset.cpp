#include "PCH.h"

#include "Viewport/EditorViewportViewModePreset.h"

Visualization ResolveEditorViewportVisualization(EditorViewportViewMode viewMode) noexcept
{
	switch (viewMode)
	{
		case EditorViewportViewMode::Lit:
		case EditorViewportViewMode::ReferencePathTracer:
			return Visualization::Lit;
		case EditorViewportViewMode::Wireframe:
			return Visualization::Wireframe;
		case EditorViewportViewMode::GBufferDiffuse:
			return Visualization::GBufferDiffuse;
		case EditorViewportViewMode::GBufferNormal:
			return Visualization::GBufferNormal;
		case EditorViewportViewMode::GBufferRoughness:
			return Visualization::GBufferRoughness;
		case EditorViewportViewMode::GBufferMetallic:
			return Visualization::GBufferMetallic;
		case EditorViewportViewMode::GBufferEmissive:
			return Visualization::GBufferEmissive;
		case EditorViewportViewMode::GBufferAmbientOcclusion:
			return Visualization::GBufferAmbientOcclusion;
		case EditorViewportViewMode::GBufferSubsurfaceColor:
			return Visualization::GBufferSubsurfaceColor;
		case EditorViewportViewMode::GBufferSubsurfaceStrength:
			return Visualization::GBufferSubsurfaceStrength;
		case EditorViewportViewMode::DirectDiffuse:
			return Visualization::DirectDiffuse;
		case EditorViewportViewMode::DirectSpecular:
			return Visualization::DirectSpecular;
		case EditorViewportViewMode::DirectSubsurface:
			return Visualization::DirectSubsurface;
		case EditorViewportViewMode::IndirectDiffuse:
			return Visualization::IndirectDiffuse;
		case EditorViewportViewMode::IndirectSpecular:
			return Visualization::IndirectSpecular;
		case EditorViewportViewMode::GpuSceneInstances:
			return Visualization::GpuSceneInstances;
		case EditorViewportViewMode::Count:
			return Visualization::Lit;
	}

	return Visualization::Lit;
}
