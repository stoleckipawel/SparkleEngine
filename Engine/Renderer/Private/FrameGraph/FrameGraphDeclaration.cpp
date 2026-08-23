#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"
#include <algorithm>
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

void FrameGraph::ApplyPassParameterDefaults()
{
	for (const PassParameterSetupCallback& setup : m_passParameterSetups)
	{
		setup();
	}
}

void FrameGraph::ApplyResourceProductionSetups()
{
	for (const ResourceProductionSetupCallback& setup : m_resourceProductionSetups)
	{
		setup();
	}
}

bool FrameGraph::HasBeenProduced(FrameGraphResourceHandle handle) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return false;
	}

	if (m_resourceRegistry.GetMetadata(handle).hasExternalContents)
	{
		return true;
	}

	const auto resource = std::find_if(
	    m_compiledPlan.resources.begin(),
	    m_compiledPlan.resources.end(),
	    [handle](const FrameGraphResourceNode& candidate) { return candidate.handle == handle; });
	if (resource == m_compiledPlan.resources.end())
	{
		return false;
	}

	return std::any_of(
	    resource->versions.begin(),
	    resource->versions.end(),
	    [this](const FrameGraphResourceVersion& version)
	    {
		    return version.writerPass != INVALID_FRAME_GRAPH_PASS_INDEX
		        && version.writerPass < m_compiledPlan.passes.size()
		        && m_compiledPlan.passes[version.writerPass].alive;
	    });
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
