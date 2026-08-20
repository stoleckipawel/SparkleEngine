#include "PCH.h"
#include "MaterialHandleResolver.h"

#include "Core/Public/Diagnostics/Verify.h"

static const auto g_materialHandleResolverLogger = Logging::GetOrCreateLogger("Renderer.MaterialHandleResolver");

namespace MaterialHandleResolver
{
	std::uint32_t ResolveSlot(MaterialHandle handle, std::uint32_t materialGeneration, std::size_t materialCount)
	{
		if (!handle.IsValid() || handle.GetGeneration() != materialGeneration || handle.GetIndex() >= materialCount)
			Diagnostics::Fatal(g_materialHandleResolverLogger, __FILE__, __LINE__, "Render object references an invalid material handle.");
		return handle.GetIndex();
	}
}
