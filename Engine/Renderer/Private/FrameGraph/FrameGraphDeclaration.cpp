#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <utility>

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

void FrameGraph::BeginFrame()
{
	// Recording has joined before the next frame; GPU-owned resources remain in this graph generation.
	m_passes.clear();
	m_passPreparations.clear();
	m_allocatedParameterInstances.clear();
	m_productRoots.clear();
}

void FrameGraph::PreparePasses()
{
	for (const auto& [passIndex, prepare] : m_passPreparations)
	{
		if (passIndex < m_compiledPlan.passes.size() && m_compiledPlan.passes[passIndex].alive)
		{
			prepare();
		}
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
		        .diagnosticName = FormatPassDiagnosticName(pass.name),
		        .eventScopeLabel = FormatPassEventScopeLabel(static_cast<FrameGraphPassIndex>(passIndex), pass.name, pass.kind),
		        .declarations = std::move(declarations),
		        .executionModel = pass.executionModel});
	}
}
