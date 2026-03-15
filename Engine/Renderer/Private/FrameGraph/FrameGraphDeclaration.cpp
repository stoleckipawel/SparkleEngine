#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Public/FrameContext.h"

#include "Core/Public/Diagnostics/Log.h"

#include <cassert>
#include <string>

namespace
{
	std::string BuildPassDisplayLabel(FrameGraph::PassIndex passIndex, std::string_view passName)
	{
		return std::string{"#"} + std::to_string(passIndex) + ":" + std::string(passName);
	}

	std::string BuildPassEventScopeLabel(FrameGraph::PassIndex passIndex, std::string_view passName, FrameGraphPassFlags flags)
	{
		std::string label{"FG/"};
		label += FrameGraphPassKindToString(flags);
		label += "/";
		label += std::to_string(passIndex);
		label += "/";
		label.append(passName.begin(), passName.end());
		return label;
	}

	void ValidatePassDeclarations(std::string_view passName,
	                              FrameGraphPassFlags flags,
	                              const std::vector<PassResourceDeclaration>& declarations) noexcept
	{
		assert(HasAnyPassFlags(flags, FrameGraphPassFlags::Raster));

		for (const PassResourceDeclaration& declaration : declarations)
		{
			if (IsReadOnlyUsage(declaration.usage) || IsWriteOnlyUsage(declaration.usage))
			{
				continue;
			}

			std::string message{"FrameGraph pass '"};
			message.append(passName.begin(), passName.end());
			message += "' uses unsupported resource usage ";
			message += ResourceUsageToString(declaration.usage);
			message += ".";
			LOG_WARNING(message);
			assert(false);
		}
	}
}

void FrameGraph::AddLambdaPass(
	std::string_view name,
	FrameGraphPassFlags flags,
	SetupCallback setupCallback,
	ExecuteCallback executeCallback)
{
	assert(HasExactlyOnePassKind(flags));
	m_passes.push_back(RegisteredPass{std::string(name), flags, std::move(setupCallback), std::move(executeCallback)});
}

void FrameGraph::BeginPassSetup() noexcept
{
	m_isSettingUpPass = true;
	m_activePassDeclarations.clear();
}

void FrameGraph::EndPassSetup() noexcept
{
	m_isSettingUpPass = false;
}

void FrameGraph::RecordDeclaration(PassResourceDeclaration declaration) noexcept
{
	m_activePassDeclarations.push_back(declaration);
}

void FrameGraph::Setup(const FrameContext& frame)
{
	m_compiledPlan.Clear();
	m_compiledPlan.passes.reserve(m_passes.size());

	for (std::size_t passIndex = 0; passIndex < m_passes.size(); ++passIndex)
	{
		auto& pass = m_passes[passIndex];
		PassBuilder builder(*this);
		BeginPassSetup();
		pass.setupCallback(builder, frame);
		EndPassSetup();
		ValidatePassDeclarations(pass.name, pass.flags, m_activePassDeclarations);
		m_compiledPlan.passes.push_back(
		    CompilePassRecord{
		        .index = static_cast<PassIndex>(passIndex),
		        .passName = pass.name,
		        .flags = pass.flags,
		        .passKind = GetFrameGraphPassKind(pass.flags),
		        .displayLabel = BuildPassDisplayLabel(static_cast<PassIndex>(passIndex), pass.name),
		        .eventScopeLabel = BuildPassEventScopeLabel(static_cast<PassIndex>(passIndex), pass.name, pass.flags),
		        .declarations = m_activePassDeclarations});
	}

	m_activePassDeclarations.clear();
}

ResourceHandle FrameGraph::Read(ResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_isSettingUpPass);
	assert(IsReadOnlyUsage(usage));
	RecordDeclaration(PassResourceDeclaration{.handle = handle, .usage = usage});
	return handle;
}

ResourceHandle FrameGraph::Write(ResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_isSettingUpPass);
	assert(IsWriteOnlyUsage(usage));
	RecordDeclaration(PassResourceDeclaration{.handle = handle, .usage = usage});
	return handle;
}