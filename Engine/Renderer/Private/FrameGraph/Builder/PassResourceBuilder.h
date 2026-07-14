#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphBufferHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <string_view>
#include <vector>

class PassResourceBuilder final
{
  public:
	explicit PassResourceBuilder(std::vector<PassResourceDeclaration>& declarations) noexcept;
	~PassResourceBuilder() noexcept = default;

	PassResourceBuilder(const PassResourceBuilder&) = delete;
	PassResourceBuilder& operator=(const PassResourceBuilder&) = delete;
	PassResourceBuilder(PassResourceBuilder&&) = delete;
	PassResourceBuilder& operator=(PassResourceBuilder&&) = delete;

	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Read(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Write(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Use(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Read(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Write(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Use(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphAccelerationStructureHandle Read(
	    FrameGraphAccelerationStructureHandle handle,
	    ResourceUsage usage,
	    std::string_view label) noexcept;
	FrameGraphAccelerationStructureHandle Write(
	    FrameGraphAccelerationStructureHandle handle,
	    ResourceUsage usage,
	    std::string_view label) noexcept;
	FrameGraphAccelerationStructureHandle Use(
	    FrameGraphAccelerationStructureHandle handle,
	    ResourceUsage usage,
	    std::string_view label) noexcept;
	bool DeclareParameterUsages(const PassParameterSet& parameterSet, std::string_view passName = {}) noexcept;

  private:
	void Record(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	static ResourceUsage GetFrameGraphUsage(const PassParameterDesc& parameter) noexcept;
	void DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareAccelerationStructureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareResourceHandle(
	    FrameGraphResourceHandle handle,
	    ResourceUsage usage,
	    const PassParameterDesc& parameter,
	    std::uint32_t arrayIndex) noexcept;

	std::vector<PassResourceDeclaration>* m_declarations = nullptr;
};
