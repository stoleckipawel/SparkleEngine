#include "PCH.h"
#include "World/Extraction/Resources/RenderResourcePublisher.h"

#include "GameFramework/Public/World/SkyEnvironment.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/SceneMeshInstanceData.h"

namespace ECS
{
	void RenderResourcePublisher::BeginScene() noexcept
	{
		m_materialRevision = 0;
		m_textureRevision = 0;
		m_skyTexturePath.reset();
		m_sky.reset();
		m_skyPublished = false;
	}

	void RenderResourcePublisher::Publish(
	    std::span<const SceneMeshInstanceGroupData> instanceGroups,
	    const std::optional<SkyEnvironment>& sky,
	    GameWorldResourceStores& resources,
	    RenderSceneDelta& delta)
	{
		PublishInstanceGroups(instanceGroups, delta);
		const std::optional<SceneSkyDesc> skyDescription = sky ? std::optional<SceneSkyDesc>(sky->Description) : std::nullopt;
		PublishSky(skyDescription, delta);

		std::optional<std::filesystem::path> skyTexturePath;
		if (sky && sky->Description.skyTexture.IsValid())
			skyTexturePath.emplace(sky->Description.skyTexture.texturePath);
		PublishChangedTables(resources, skyTexturePath, delta);
	}

	void RenderResourcePublisher::PublishInstanceGroups(
	    std::span<const SceneMeshInstanceGroupData> instanceGroups,
	    RenderSceneDelta& delta) const
	{
		if (!delta.ResetScene)
			return;
		delta.InstanceGroups.Published = true;
		delta.InstanceGroups.Values.reserve(instanceGroups.size());
		for (const SceneMeshInstanceGroupData& group : instanceGroups)
			delta.InstanceGroups.Values.push_back(
			    {group.meshAssetId,
			        group.meshAssetIndex,
			        group.materialHandle,
			        group.firstInstance,
			        group.instanceCount,
			        group.groupKind,
			        group.flags});
	}

	void RenderResourcePublisher::PublishSky(const std::optional<SceneSkyDesc>& sky, RenderSceneDelta& delta)
	{
		if (!delta.ResetScene && m_skyPublished && HasSameSky(sky, m_sky))
			return;
		delta.Sky.Published = true;
		delta.Sky.Value = sky;
		m_sky = sky;
		m_skyPublished = true;
	}

	void RenderResourcePublisher::PublishChangedTables(
	    GameWorldResourceStores& resources,
	    const std::optional<std::filesystem::path>& skyTexturePath,
	    RenderSceneDelta& delta)
	{
		const std::uint64_t materialRevision = resources.Materials.GetContentRevision();
		if (delta.ResetScene || materialRevision != m_materialRevision)
		{
			delta.Materials = resources.Materials.CaptureRenderTable();
			m_materialRevision = materialRevision;
		}

		const std::uint64_t textureRevision = resources.Textures.GetContentRevision();
		if (delta.ResetScene || textureRevision != m_textureRevision || skyTexturePath != m_skyTexturePath)
		{
			if (skyTexturePath)
				delta.Textures = resources.Textures.CaptureRenderTable(std::span<const std::filesystem::path>(&*skyTexturePath, 1));
			else
				delta.Textures = resources.Textures.CaptureRenderTable();
			m_textureRevision = textureRevision;
			m_skyTexturePath = skyTexturePath;
		}
	}

	bool RenderResourcePublisher::HasSameSky(const std::optional<SceneSkyDesc>& left, const std::optional<SceneSkyDesc>& right) noexcept
	{
		if (left.has_value() != right.has_value())
			return false;
		if (!left)
			return true;
		return left->enabled == right->enabled && left->color.x == right->color.x && left->color.y == right->color.y
		    && left->color.z == right->color.z && left->brightness == right->brightness
		    && left->skyTexture.texturePath == right->skyTexture.texturePath
		    && left->skyTexture.textureGroup == right->skyTexture.textureGroup;
	}
}
