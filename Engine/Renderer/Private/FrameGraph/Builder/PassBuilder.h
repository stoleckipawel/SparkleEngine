#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

class FrameGraph;

class PassBuilder final
{
  public:
	explicit PassBuilder(FrameGraph& frameGraph) noexcept;
	~PassBuilder() noexcept = default;

	PassBuilder(const PassBuilder&) = delete;
	PassBuilder& operator=(const PassBuilder&) = delete;
	PassBuilder(PassBuilder&&) = delete;
	PassBuilder& operator=(PassBuilder&&) = delete;

	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Read(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Write(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Use(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Read(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Write(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Use(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	void DeclareParameterUsages(const PassParameterSet& parameterSet) noexcept;

  private:
	static bool HasFrameGraphUsage(const PassParameterDesc& parameter) noexcept;
	static ResourceUsage GetFrameGraphUsage(const PassParameterDesc& parameter) noexcept;
	void DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareResourceHandle(
	    FrameGraphResourceHandle handle,
	    ResourceUsage usage,
	    const PassParameterDesc& parameter,
	    std::uint32_t arrayIndex) noexcept;

	FrameGraph* m_frameGraph = nullptr;
};