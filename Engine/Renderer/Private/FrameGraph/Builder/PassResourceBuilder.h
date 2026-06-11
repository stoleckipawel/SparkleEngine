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
	FrameGraphResourceHandle Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphResourceHandle Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Read(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Write(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Use(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphTextureHandle Read(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Write(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphTextureHandle Use(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Read(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Write(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Use(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept;
	FrameGraphBufferHandle Read(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Write(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphBufferHandle Use(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept;
	FrameGraphAccelerationStructureHandle Read(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphAccelerationStructureHandle Write(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept;
	FrameGraphAccelerationStructureHandle Use(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept;
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
	static bool HasFrameGraphUsage(const PassParameterDesc& parameter) noexcept;
	static ResourceUsage GetFrameGraphUsage(const PassParameterDesc& parameter) noexcept;
	void DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareAccelerationStructureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept;
	void DeclareResourceHandle(
	    FrameGraphResourceHandle handle,
	    ResourceUsage usage,
	    const PassParameterDesc& parameter,
	    std::uint32_t arrayIndex) noexcept;

	PassResourceDeclarationSink* m_declarations = nullptr;
};
