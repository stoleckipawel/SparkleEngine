#pragma once

#include "RHI/Public/Shaders/CookedShaderPackageUtils.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Engine::AssetAuthoring
{
	inline constexpr std::string_view kShaderCookManifestFileName = "ShaderPackages.ini";

	struct ShaderCookStageDesc
	{
		ShaderStage stage = ShaderStage::Count;
		std::filesystem::path sourcePath;
		std::string entryPoint = "main";
	};

	struct ShaderCookPackageDesc
	{
		std::string packageId;
		std::string bindingLayoutId;
		std::string variantId = "Default";
		std::vector<ShaderCookStageDesc> stages;
	};

	class ShaderCookManifest final
	{
	  public:
		bool LoadMerged(std::string& outErrorMessage);

		const std::vector<ShaderCookPackageDesc>& GetPackages() const noexcept { return m_packages; }

		static std::filesystem::path GetEngineManifestPath();
		static std::filesystem::path GetProjectManifestPath();
		static std::filesystem::path GetCookedShaderRoot();
		static std::filesystem::path GetCookedShaderPackageRoot();
		static std::filesystem::path GetCookedShaderRegistryPath();
		static std::uint64_t BuildShaderPackageKey(std::string_view packageId, std::string_view variantId = "Default");
		static std::filesystem::path BuildCookedShaderPackagePath(std::uint64_t packageKey);

	  private:
		std::vector<ShaderCookPackageDesc> m_packages;
	};
}  // namespace Engine::AssetAuthoring