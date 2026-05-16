#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <string_view>
#include <vector>

class PassResourceDeclarationSink final
{
  public:
	explicit PassResourceDeclarationSink(std::vector<PassResourceDeclaration>& declarations) noexcept;

	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label = {}) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label = {}) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label = {}) noexcept;

  private:
	void Record(PassResourceDeclaration declaration) noexcept;

	std::vector<PassResourceDeclaration>* m_declarations = nullptr;
};

class PassResourceBuilder final
{
  public:
	explicit PassResourceBuilder(PassResourceDeclarationSink& declarations) noexcept;
	~PassResourceBuilder() noexcept = default;

	PassResourceBuilder(const PassResourceBuilder&) = delete;
	PassResourceBuilder& operator=(const PassResourceBuilder&) = delete;
	PassResourceBuilder(PassResourceBuilder&&) = delete;
	PassResourceBuilder& operator=(PassResourceBuilder&&) = delete;

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

	PassResourceDeclarationSink* m_declarations = nullptr;
};
