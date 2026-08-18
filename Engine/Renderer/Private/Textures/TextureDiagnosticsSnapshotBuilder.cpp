#include "../PCH.h"
#include "Textures/TextureDiagnosticsSnapshotBuilder.h"

#include "RHI/Public/Descriptors/RhiDescriptorService.h"
#include "Textures/RendererTexture.h"

#include <algorithm>
#include <utility>

TextureDiagnosticsSnapshotBuilder::TextureDiagnosticsSnapshotBuilder(
    const RhiDescriptorService& descriptorService,
    const PreviewTextureResolver& resolvePreviewTexture,
    std::size_t expectedRowCount) :
    m_descriptorService(descriptorService),
    m_resolvePreviewTexture(resolvePreviewTexture)
{
	m_snapshot.Rows.reserve(expectedRowCount);
}

void TextureDiagnosticsSnapshotBuilder::Add(
    const RendererTexture& texture,
    TextureDiagnosticsKind kind,
    std::string key,
    bool streamManaged)
{
	if (!texture)
	{
		return;
	}

	TextureDiagnosticsRow row;
	row.Key = std::move(key);
	row.Kind = kind;
	row.Dimension = texture.Dimension;
	row.FormatIntent = texture.FormatIntent;
	row.ResidencyState = TextureDiagnosticsResidencyState::Resident;
	row.Width = texture.Width;
	row.Height = texture.Height;
	row.ArraySize = texture.ArraySize;
	row.Format = PixelFormatName(texture.Format);
	row.MipCount = texture.MipCount;
	row.EstimatedByteSize = texture.EstimatedByteSize;
	const std::uint64_t nativeTextureId = m_descriptorService.GetResourceViewGpuHandle(texture.ShaderResourceView).Value;
	row.PreviewTexture = m_resolvePreviewTexture ? m_resolvePreviewTexture(nativeTextureId) : EditorTextureHandle{};
	row.Loaded = true;
	row.StreamManaged = streamManaged;
	m_snapshot.Rows.push_back(std::move(row));
}

TextureDiagnosticsSnapshot TextureDiagnosticsSnapshotBuilder::Build() &&
{
	std::sort(
	    m_snapshot.Rows.begin(),
	    m_snapshot.Rows.end(),
	    [](const TextureDiagnosticsRow& lhs, const TextureDiagnosticsRow& rhs) noexcept
	    {
		    if (lhs.Kind != rhs.Kind)
		    {
			    return static_cast<std::uint8_t>(lhs.Kind) < static_cast<std::uint8_t>(rhs.Kind);
		    }
		    return lhs.Key < rhs.Key;
	    });
	return std::move(m_snapshot);
}
