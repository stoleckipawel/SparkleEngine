#include "PCH.h"

#include "RenderSceneDataBuilder.h"

#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderWorld.h"
#include "SceneData/Builders/RenderLightingBuilder.h"
#include "SceneData/Builders/RenderMeshDrawBuilder.h"
#include "SceneData/Caching/MaterialCacheManager.h"
#include "Textures/TextureManager.h"
#include "Textures/RendererTexture.h"

static const auto g_renderSceneDataBuilderLogger = Logging::GetOrCreateLogger("Renderer.SceneData");

RenderSceneDataBuilder::RenderSceneDataBuilder(
    MaterialCacheManager& materialCache,
    GPUMeshCache& gpuMeshCache,
    TextureManager& textureManager) noexcept :
    m_materialCache(&materialCache), m_gpuMeshCache(&gpuMeshCache), m_textureManager(&textureManager),
	m_meshDrawBuilder(std::make_unique<RenderMeshDrawBuilder>(gpuMeshCache))
{
}

RenderSceneDataBuilder::~RenderSceneDataBuilder() noexcept = default;

RenderSceneData RenderSceneDataBuilder::Build(const RenderWorld& world, const RenderFrameDynamicData& dynamic)
{
	RenderSceneData sceneData = {};

	if (!m_materialCache)
	{
		Diagnostics::Fail(
		    g_renderSceneDataBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "RenderSceneDataBuilder::Build: material cache manager is unavailable.");
		return sceneData;
	}

	BuildMaterials(world, sceneData);
	m_meshDrawBuilder->Build(world, dynamic, sceneData);
	BuildSky(world, sceneData);
	RenderLightingBuilder::Build(dynamic.Lights, sceneData);
	return sceneData;
}

void RenderSceneDataBuilder::BuildSky(const RenderWorld& world, RenderSceneData& sceneData) const
{
	if (m_textureManager == nullptr)
	{
		return;
	}

	const RendererTexture* skyTexture = nullptr;
	const SceneSkyDesc* sky = world.GetSky() ? &*world.GetSky() : nullptr;
	if (sky == nullptr)
	{
		skyTexture = m_textureManager->ResolveDefaultSkyTexture();
	}
	else
	{
		sceneData.sky.enabled = sky->enabled;
		sceneData.sky.color = sky->color;
		sceneData.sky.intensity = sky->intensity;
		if (!sky->skyTexture.IsValid())
		{
			skyTexture = m_textureManager->ResolveDefaultSkyTexture();
		}
		else
		{
			skyTexture = m_textureManager->GetSceneTexture(sky->skyTexture.texturePath);
			if (skyTexture == nullptr)
			{
				skyTexture = m_textureManager->GetTexture(TextureId::Checker);
				SPDLOG_LOGGER_ERROR(
				    g_renderSceneDataBuilderLogger,
				    "RenderSceneDataBuilder: level sky texture '{}' is unavailable; using the diagnostic checker texture.",
				    sky->skyTexture.texturePath);
			}
		}
	}

	sceneData.sky.texture = skyTexture != nullptr && *skyTexture ? skyTexture : nullptr;
}

void RenderSceneDataBuilder::BuildMaterials(const RenderWorld& world, RenderSceneData& sceneData) const
{
	if (m_materialCache == nullptr)
	{
		return;
	}

	m_materialCache->BuildMaterials(world.GetMaterials(), sceneData);
}
