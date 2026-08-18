#pragma once

#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"

#include <cstddef>
#include <string>

struct RendererTexture;
class RhiDescriptorService;

class TextureDiagnosticsSnapshotBuilder final
{
public:
	using PreviewTextureResolver = TexturePreviewHandleResolver;

	TextureDiagnosticsSnapshotBuilder(
	    const RhiDescriptorService& descriptorService,
	    const PreviewTextureResolver& resolvePreviewTexture,
	    std::size_t expectedRowCount);

	void Add(const RendererTexture& texture, TextureDiagnosticsKind kind, std::string key, bool streamManaged);
	TextureDiagnosticsSnapshot Build() &&;

private:
	const RhiDescriptorService& m_descriptorService;
	const PreviewTextureResolver& m_resolvePreviewTexture;
	TextureDiagnosticsSnapshot m_snapshot;
};
