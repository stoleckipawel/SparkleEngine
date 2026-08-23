#include "PCH.h"
#include "UI/UiFrameRenderer.h"

#include "Editor/EditorTextureRegistry.h"
#include "Frame/Graph/RenderProductGraphHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "UI/UiRenderPacketPlayer.h"

UiFrameRenderer::UiFrameRenderer(RenderDeviceServices& deviceServices, bool ownsBackend) :
    m_deviceServices(deviceServices),
    m_packetPlayer(std::make_unique<UiRenderPacketPlayer>()),
    m_textureRegistry(std::make_unique<EditorTextureRegistry>()),
    m_ownsBackend(ownsBackend)
{
	if (m_ownsBackend)
	{
		m_deviceServices.GetImGuiRenderer().Initialize();
	}
}

UiFrameRenderer::~UiFrameRenderer() noexcept
{
	if (m_ownsBackend)
	{
		m_packetPlayer->Shutdown(m_deviceServices.GetImGuiRenderer());
		m_deviceServices.GetImGuiRenderer().Shutdown();
	}
}

EditorTextureHandle UiFrameRenderer::RegisterEditorTexture(std::uint64_t nativeTextureId) noexcept
{
	return m_textureRegistry->Register(nativeTextureId);
}

void UiFrameRenderer::BeginFrame() noexcept
{
	if (m_ownsBackend)
	{
		m_deviceServices.GetImGuiRenderer().BeginFrame();
	}
}

void UiFrameRenderer::Render(
    const UiRenderPacket& packet,
    FrameGraph* frameGraph,
    ViewportRenderProducts& viewportProducts) noexcept
{
	switch (packet.PresentationMode)
	{
		case UiPresentationMode::HostOverlay:
			RenderHostOverlay(packet);
			break;
		case UiPresentationMode::EditorViewport:
			RenderEditorViewport(packet, frameGraph, viewportProducts);
			break;
		case UiPresentationMode::None:
		default:
			break;
	}
}

bool UiFrameRenderer::BeginViewportPresentation(FrameGraph& frameGraph, ViewportRenderProducts& viewportProducts) noexcept
{
	const RenderProduct* product = viewportProducts.FindProduct(RenderOutputFlags::SceneColor);
	if (product == nullptr || !product->Handle)
	{
		return false;
	}

	RenderProduct publishedProduct = *product;
	TransitionViewportProduct(frameGraph, viewportProducts, ResourceState::ShaderResource);
	const FrameGraphResourceHandle resource = ToFrameGraphResourceHandle(product->Handle);
	const std::uint64_t textureId = m_deviceServices.GetImGuiRenderer().ResolveTextureId(
	    frameGraph.ResolveShaderResourceView(FrameGraphTextureHandle{resource}));
	if (textureId == 0u)
	{
		TransitionViewportProduct(frameGraph, viewportProducts, ResourceState::Common);
		return false;
	}

	publishedProduct.EditorTexture = m_textureRegistry->PublishViewportTexture(textureId, viewportProducts.GetGeneration());
	viewportProducts.SetProduct(RenderOutputFlags::SceneColor, publishedProduct);
	return true;
}

void UiFrameRenderer::EndViewportPresentation(FrameGraph& frameGraph, const ViewportRenderProducts& viewportProducts) noexcept
{
	TransitionViewportProduct(frameGraph, viewportProducts, ResourceState::Common);
}

void UiFrameRenderer::RenderEditorViewport(
    const UiRenderPacket& packet,
    FrameGraph* frameGraph,
    ViewportRenderProducts& viewportProducts) noexcept
{
	if (frameGraph == nullptr || !BeginViewportPresentation(*frameGraph, viewportProducts))
	{
		m_textureRegistry->RetireViewportTexture();
		return;
	}

	if (!packet.HasDrawData() || packet.ViewportGeneration != viewportProducts.GetGeneration())
	{
		EndViewportPresentation(*frameGraph, viewportProducts);
		return;
	}

	constexpr float clearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
	RhiPresentationService& presentation = m_deviceServices.GetRenderHardwareInterface().GetPresentationService();
	presentation.BeginPresentRenderPass(clearColor);
	Play(packet);
	presentation.EndPresentRenderPass();
	EndViewportPresentation(*frameGraph, viewportProducts);
}

void UiFrameRenderer::RenderHostOverlay(const UiRenderPacket& packet) noexcept
{
	if (!packet.HasDrawData())
	{
		return;
	}

	RhiPresentationService& presentation = m_deviceServices.GetRenderHardwareInterface().GetPresentationService();
	presentation.BeginPresentOverlayPass();
	Play(packet);
	presentation.EndPresentRenderPass();
}

void UiFrameRenderer::Play(const UiRenderPacket& packet) noexcept
{
	m_packetPlayer->Render(packet, *m_textureRegistry, m_deviceServices.GetImGuiRenderer());
}

void UiFrameRenderer::TransitionViewportProduct(
    FrameGraph& frameGraph,
    const ViewportRenderProducts& viewportProducts,
    ResourceState after) noexcept
{
	const RenderProduct* product = viewportProducts.FindProduct(RenderOutputFlags::SceneColor);
	if (product == nullptr || !product->Handle)
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = ToFrameGraphResourceHandle(product->Handle);
	const RhiResourceHandle resource = frameGraph.ResolveResource(FrameGraphTextureHandle{resourceHandle});
	if (!resource)
	{
		return;
	}

	const ResourceState before = frameGraph.GetTrackedResourceState(resourceHandle);
	if (before == after)
	{
		return;
	}

	RenderHardwareInterface& rhi = m_deviceServices.GetRenderHardwareInterface();
	RenderCommandList& commandList = m_deviceServices.GetGraphicsCommandList(rhi.GetCurrentFrameIndex());
	commandList.TransitionResource(resource, before, after);
	frameGraph.UpdateTrackedResourceState(resourceHandle, after);
}
