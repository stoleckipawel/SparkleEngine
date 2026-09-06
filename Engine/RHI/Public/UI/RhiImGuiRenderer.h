#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../RHIAPI.h"

#include <cstdint>

struct ImDrawData;
struct ImTextureData;

class SPARKLE_RHI_API RhiImGuiRenderer
{
public:
	virtual ~RhiImGuiRenderer() noexcept = default;
	RhiImGuiRenderer(const RhiImGuiRenderer&) = delete;
	RhiImGuiRenderer& operator=(const RhiImGuiRenderer&) = delete;
	RhiImGuiRenderer(RhiImGuiRenderer&&) = delete;
	RhiImGuiRenderer& operator=(RhiImGuiRenderer&&) = delete;

	virtual void Initialize() = 0;
	virtual void BeginFrame() noexcept = 0;
	virtual std::uint64_t ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept = 0;
	virtual void RenderDrawData(ImDrawData* drawData) noexcept = 0;
	virtual void ReleaseTexture(ImTextureData& texture) noexcept = 0;
	virtual void Shutdown() noexcept = 0;

protected:
	RhiImGuiRenderer() noexcept = default;
};
