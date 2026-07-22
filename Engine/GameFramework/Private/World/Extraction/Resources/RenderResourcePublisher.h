#pragma once

#include "GameFramework/Public/Rendering/RenderWorldDelta.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>

class GameWorldResourceStores;
struct SceneMeshInstanceGroupData;
struct SkyEnvironment;

namespace ECS
{
	class RenderResourcePublisher final
	{
	  public:
		void BeginScene() noexcept;
		void Publish(
		    std::span<const SceneMeshInstanceGroupData> instanceGroups,
		    const std::optional<SkyEnvironment>& sky,
		    GameWorldResourceStores& resources,
		    RenderWorldDelta& delta);

	  private:
		void PublishInstanceGroups(
		    std::span<const SceneMeshInstanceGroupData> instanceGroups,
		    RenderWorldDelta& delta) const;
		void PublishSky(const std::optional<SceneSkyDesc>& sky, RenderWorldDelta& delta);
		void PublishChangedTables(
		    GameWorldResourceStores& resources,
		    const std::optional<std::filesystem::path>& skyTexturePath,
		    RenderWorldDelta& delta);
		static bool HasSameSky(
		    const std::optional<SceneSkyDesc>& left,
		    const std::optional<SceneSkyDesc>& right) noexcept;

		std::uint64_t m_materialRevision = 0;
		std::uint64_t m_textureRevision = 0;
		std::optional<std::filesystem::path> m_skyTexturePath;
		std::optional<SceneSkyDesc> m_sky;
		bool m_skyPublished = false;
	};
}
