#pragma once

#include "RHI/Public/D3D12/Shaders/ShaderCompileOptions.h"

#include <filesystem>
#include <string>

class ShaderSourceDefinition final
{
  public:
	ShaderSourceDefinition() = default;

	static ShaderSourceDefinition FromAsset(std::filesystem::path sourcePath, std::string entryPoint, ShaderStage stage)
	{
		ShaderSourceDefinition definition;
		definition.m_sourcePath = std::move(sourcePath);
		definition.m_entryPoint = std::move(entryPoint);
		definition.m_stage = stage;
		return definition;
	}

	const std::filesystem::path& GetSourcePath() const noexcept { return m_sourcePath; }

	const std::string& GetEntryPoint() const noexcept { return m_entryPoint; }

	ShaderStage GetStage() const noexcept { return m_stage; }

	bool IsValid() const noexcept { return !m_sourcePath.empty() && !m_entryPoint.empty() && m_stage != ShaderStage::Count; }

	explicit operator bool() const noexcept { return IsValid(); }

  private:
	std::filesystem::path m_sourcePath;
	std::string m_entryPoint;
	ShaderStage m_stage = ShaderStage::Count;
};