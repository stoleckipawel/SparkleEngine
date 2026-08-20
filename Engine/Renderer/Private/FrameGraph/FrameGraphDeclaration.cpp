#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"
#include <cassert>
#include <string>
#include <utility>

class FrameGraphPassLabelFormatter final
{
public:
	static std::string FormatPassEventScopeLabel(FrameGraphPassIndex passIndex, std::string_view passName, EFrameGraphPassKind passKind)
	{
		std::string label{"FrameGraph/"};
		label += FrameGraphPassKindToString(passKind);
		label += "/";
		label += std::to_string(passIndex);
		label += "/";
		label.append(passName.begin(), passName.end());
		return label;
	}

	static std::string FormatPassDiagnosticName(std::string_view passName)
	{
		std::string name{"Renderer.FrameGraph."};
		name.append(passName.begin(), passName.end());
		return name;
	}
};

void FrameGraph::ApplyFrameUniformParameters(const FrameUniformData& frame)
{
	for (const FrameUniformSetupCallback& setup : m_frameUniformSetups)
	{
		setup(frame);
	}
}

void FrameGraph::ApplyPreparedSceneParameters(const PreparedRenderScene& preparedScene)
{
	for (const PreparedSceneSetupCallback& setup : m_preparedSceneSetups)
	{
		setup(preparedScene);
	}
}

void FrameGraph::ApplyRenderViewParameters(const RenderView& view)
{
	for (const RenderViewSetupCallback& setup : m_renderViewSetups)
	{
		setup(view);
	}
}

void FrameGraph::ApplyExposureParameters(const ExposureUniformData& exposure)
{
	for (const ExposureSetupCallback& setup : m_exposureSetups)
	{
		setup(exposure);
	}
}

void FrameGraph::ApplyToneMappingParameters(const ToneMappingUniformData& toneMapping)
{
	for (const ToneMappingSetupCallback& setup : m_toneMappingSetups)
	{
		setup(toneMapping);
	}
}

void FrameGraph::ApplyDirectLightReservoirHistory(bool historyValid)
{
	for (const HistorySetupCallback& setup : m_directLightReservoirHistorySetups)
	{
		setup(historyValid);
	}
}

void FrameGraph::ApplyRestirIndirectReservoirHistory(bool historyValid)
{
	for (const HistorySetupCallback& setup : m_restirIndirectReservoirHistorySetups)
	{
		setup(historyValid);
	}
}

void FrameGraph::ApplyReferenceLightingHistory(bool historyValid)
{
	for (const HistorySetupCallback& setup : m_referenceLightingHistorySetups)
	{
		setup(historyValid);
	}
}

void FrameGraph::ApplyRayTracedShadowParameters(const PreparedRenderScene& preparedScene, const RayTracedShadowPassInput& rayTracedShadows)
{
	for (const RayTracedShadowSetupCallback& setup : m_rayTracedShadowSetups)
	{
		setup(preparedScene, rayTracedShadows);
	}
}

void FrameGraph::ApplyPassParameterDefaults()
{
	for (const PassParameterSetupCallback& setup : m_passParameterSetups)
	{
		setup();
	}
}

void FrameGraph::Setup()
{
	m_compiledPlan.Clear();
	m_compiledPlan.productRoots = m_productRoots;
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		std::vector<PassResourceDeclaration> declarations;
		PassResourceBuilder builder(declarations);
		pass.active = pass.setupCallback(builder);
		FrameGraphResourceContractDiagnostics::ValidatePassDeclarations(pass.name, pass.kind, declarations);
		assert(IsQueuePreferenceCompatible(pass.kind, pass.queuePreference));
		m_compiledPlan.passes.push_back(
		    FrameGraphPassNode{
		        .index = static_cast<FrameGraphPassIndex>(passIndex),
		        .passName = pass.name,
		        .kind = pass.kind,
		        .queuePreference = pass.queuePreference,
		        .diagnosticName = FrameGraphPassLabelFormatter::FormatPassDiagnosticName(pass.name),
		        .eventScopeLabel = FrameGraphPassLabelFormatter::FormatPassEventScopeLabel(
		            static_cast<FrameGraphPassIndex>(passIndex),
		            pass.name,
		            pass.kind),
		        .declarations = std::move(declarations),
		        .executionModel = pass.executionModel});
	}
}
